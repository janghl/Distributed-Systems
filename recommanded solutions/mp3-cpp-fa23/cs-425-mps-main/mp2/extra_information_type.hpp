#pragma once 

class ExtraInformation {
public:
        virtual void init() {}
        virtual void lock() {}
        virtual void unlock() {}
        virtual uint32_t struct_serialized_size() const { return 0; }
        virtual char* serialize(char* buffer) const { return buffer; }
        static const char* deserialize(const char* buffer, ExtraInformation* extra_information) { return buffer; }
        virtual void merge(const ExtraInformation* merge_source) { return; }
        virtual ~ExtraInformation() {};
};


class BlockReport;

#ifdef EXTRAINFORMATION
typedef ExtraInformation ExtraInformationType;
#endif

#ifdef BLOCKREPORT
typedef BlockReport ExtraInformationType;
#endif
