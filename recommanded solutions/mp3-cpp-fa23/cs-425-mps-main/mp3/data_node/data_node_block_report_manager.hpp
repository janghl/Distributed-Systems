#pragma once
#include <arpa/inet.h>

#include <cstring>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <mutex>

#include "utils.hpp"
#include "../mp2/extra_information_type.hpp"

class BlockReport : public ExtraInformation {
private:
	std::unordered_map<std::string, uint32_t> files_metadata; 	// mapping all file names on current machine to their udpate counter 
	uint32_t files_metadata_serialized_size;			// size (in bytes) of serialized files_metadata
	uint32_t update_counter;					// generation counter that is updated everytime a file is modified
	std::string file_system_root;					// directory where files are stored
	std::mutex mtx;                                 		// mutex to guard concurrent access to this class

public:
	BlockReport() : files_metadata_serialized_size(0), file_system_root(""), update_counter(0) {}

	BlockReport(const BlockReport& block) {
		this->copy_from_another_block(block);
	}

	BlockReport(const BlockReport&& block) {
		this->copy_from_another_block(block);
        }

	BlockReport& operator=(const BlockReport& other) {
                if (this == &other) {
                        return *this;
                }
                this->copy_from_another_block(other);
                return *this;
        }

	BlockReport& operator=(const BlockReport&& other) {
		if (this == &other) {
			return *this;
		}
                this->copy_from_another_block(other);
		return *this;
        }

	uint32_t get_update_counter() {
		return this->update_counter;
	}

	std::unordered_map<std::string, uint32_t> get_files_metadata() const {
		return this->files_metadata;
	}

	// Removes entry in files_metadata with key matching file_name
	// Precondition: Caller must hold a lock on BlockReport
	void delete_file(std::string file_name) {
		this->files_metadata_serialized_size -= sizeof(uint32_t) + file_name.length() + 1;
		this->files_metadata.erase(file_name);
		++this->update_counter;
	}

	// Returns a string listing the file names of all files stored on data node. File names are separated by new lines.
	std::string get_file_names() {
		std::string file_names; // file names (keys of files_metadata), separated by new lines
		lock();
		for (auto iter = files_metadata.begin(); iter != files_metadata.end(); ++iter) {
			file_names += iter->first + "\n";
		}
		unlock();
		return file_names;
	}

	void copy_from_another_block(const BlockReport& block) {
                files_metadata = block.files_metadata;
                files_metadata_serialized_size = block.files_metadata_serialized_size;
                update_counter = block.update_counter;
                file_system_root = block.file_system_root;
        }

	// Deletes all files in directory indicated by file_system_root and in future, writes all new files to file_system_root directory.
	BlockReport(std::string file_system_root) : 
		files_metadata_serialized_size(0),
		update_counter(0),
		file_system_root(file_system_root) {
			// Delete everything file_system_root
			for (const auto& entry : std::filesystem::directory_iterator(file_system_root))
				std::filesystem::remove_all(entry.path());
		}

	void lock() override {
		mtx.lock();
	}

	void unlock() override {
		mtx.unlock();
	}

	std::string get_file_system_root() {
		return this->file_system_root;
	}

	// Returns the length of buffer required to serialize this->files_metadata.
	// Precondition: Caller must hold a lock on BlockReport
	uint32_t struct_serialized_size() const override {
		return this->files_metadata_serialized_size + 
				sizeof(uint32_t) + // update_counter
				sizeof(uint32_t);  // files_metadata_serialized_size
	}

	// Adds file_name to files_metadata with update counter set to 0
	void add_new_file(std::string file_name) {
		this->lock();
		this->_add_new_file_impl(file_name, 0);
		this->unlock();
	}

	void _add_new_file_impl(std::string file_name, uint32_t file_update_counter) {
		this->files_metadata[file_name] = file_update_counter;
                this->files_metadata_serialized_size += sizeof(uint32_t) + file_name.length() + 1; // +1 for null byte at end of file_name
                ++this->update_counter;
#ifdef DEBUG_MP3_DATA_NODE_BLOCK_REPORT
		std::cout << "DATA_NODE_BLOCK_REPORT: Adding new file " << file_name << std::endl;
#endif		
	}

	// Sets the value of file_name in files_metadata to file_update_counter. 
	// Returns 0 if the file already existed and -1 if the file is new
	// Precondition: Caller must ensure that the file_update_counter is greater than or equal to the current file update counter. 
	// Otherwise, the integrity of data may be lost.
	// Precondition: The caller holds the lock on this
	int set_file_update_counter(std::string file_name, uint32_t file_update_counter) {
#ifdef DEBUG_MP3_DATA_NODE_BLOCK_REPORT
		std::cout << "DATA_NODE_BLOCK_REPORT: setting update counter of file " << file_name << std::endl;
#endif
		auto it = this->files_metadata.find(file_name);
		if (it != this->files_metadata.end()) {
			it->second = file_update_counter; 
			++this->update_counter;
		} else {
			_add_new_file_impl(file_name, file_update_counter);
		}
		int return_value = (it != this->files_metadata.end()) ? 0 : -1;
		return return_value;
	}

	// Returns the file's update counter. 
	// Precondition: The caller holds the lock on this
        uint32_t get_file_update_counter(std::string file_name) const {
		auto it = this->files_metadata.find(file_name);
                uint32_t counter = 0;
                if (it != this->files_metadata.end()) {
                        counter = it->second;
                }
                return counter;
        }

	// Serialzies this struct, including files_metadata_serialized_size and files_metadata, into buffer. Returns the  
	// Precondition: Caller must hold a lock on BlockReport
	// Precondition: The size of buffer must be >= value returned by this.struct_serialized_size()
	// Caller: The function is primarily invoked by member information class (from MP 2)
	char* serialize(char* buffer) const override {
		uint32_t network_total_size = htonl(this->struct_serialized_size());
		memcpy(buffer, (char*)&network_total_size, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		uint32_t network_update_counter = htonl(this->update_counter);
		memcpy(buffer, (char*)&network_update_counter, sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		for (auto& it: this->files_metadata) {
			// serialize the file name (it.first) followed by the file update counter (it.second)
			memcpy(buffer, it.first.c_str(), it.first.length() + 1); // +1 to copy the null byte
			buffer += it.first.length() + 1;

			uint32_t network_file_update_counter = htonl(it.second);
			memcpy(buffer, (char*)&network_file_update_counter, sizeof(uint32_t));
			buffer += sizeof(uint32_t);
		}

		return buffer;
	}

	// Deserializes the files_metadata_serialized_size and files_metadata. Updates block_report with the deserialized values 
	// (returned via argument).
	// Precondition: The size of buffer must be >= value returned by this.struct_serialized_size()
	static const char* deserialize(const char* buffer, BlockReport* block_report) {
		if (block_report == nullptr) {
			return buffer;
		}
		uint32_t network_total_size;
		memcpy((char*)&network_total_size, buffer, sizeof(uint32_t));
		block_report->files_metadata_serialized_size = ntohl(network_total_size) - (2 * sizeof(uint32_t));
		buffer += sizeof(uint32_t);

		uint32_t network_update_counter;
		memcpy((char*)&network_update_counter, buffer, sizeof(uint32_t));
                block_report->update_counter = ntohl(network_update_counter);
                buffer += sizeof(uint32_t);

		uint32_t bytes_read = sizeof(uint32_t);
		while (bytes_read < block_report->files_metadata_serialized_size) {
			std::string file_name(buffer); // file_name = buffer[0:i] such that buffer[i] = '\0' and i is as small as possible
			buffer += file_name.length() + 1; // +1 to skip over the null byte
			bytes_read += file_name.length() + 1;

			uint32_t network_file_update_counter;
			memcpy((char*)&network_file_update_counter, buffer, sizeof(uint32_t));
			block_report->files_metadata[file_name] = ntohl(network_file_update_counter);
			buffer += sizeof(uint32_t);
			bytes_read += sizeof(uint32_t);
		}

		return buffer;
	}

	// Copies all data from merge_source. 
	void merge(const ExtraInformation* extra_information) override {
		if (extra_information == nullptr) {
			return;
		}
		const BlockReport* merge_source = static_cast<const BlockReport*>(extra_information);
		this->lock();
		if (this->update_counter > merge_source->update_counter) {
			this->unlock();
			return;
		}
		this->update_counter = merge_source->update_counter;
		this->files_metadata_serialized_size = merge_source->files_metadata_serialized_size;
		this->files_metadata = merge_source->files_metadata;
		this->unlock();
	}

	// Precondition: Caller must hold a lock on this and other
	bool operator==(const BlockReport& other) const {
		return this->update_counter == other.update_counter &&
			(this->files_metadata_serialized_size == other.files_metadata_serialized_size) &&
			(this->files_metadata == other.files_metadata);
	}

	// Precondition: Caller must hold a lock on this and other
        bool operator!=(const BlockReport& other) const {
                return not (this->operator==(other));
        }
};
