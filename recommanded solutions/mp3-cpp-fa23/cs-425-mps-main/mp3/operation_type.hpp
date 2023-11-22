#pragma once

enum class OperationType {
        INVALID,        // denotes error
        READ,           // Request: READ sdfsfilename 			Response: nodeid READ sdfsfilename statuscode fileupdaatecounter filedata
        WRITE,          // Request: WRITE sdfsfilename fileupdatecounter filedata Response: nodeid WRITE sdfsfilename statuscode fileupdaatecounter
        DELETE,         // Request: DELETE sdfsfilename			Response: nodeid DELETE sdfsfilename statuscode
        LS              // Request: LS					Response: nodeid LS statuscode files
};

OperationType is_sdfs_command(std::string command);
std::string operation_to_string(OperationType operation);
