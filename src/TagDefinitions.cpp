#define _CRT_SECURE_NO_WARNINGS (1)

#include <TagDefinitions.hpp>
#include <SpeedwireData.hpp>
#include <SpeedwireStatus.hpp>
#include <vector>
#include <stdio.h>

using namespace libspeedwire;


int TagDefinitions::readFromFile(const std::string& path) {

    FILE* inp = fopen(path.c_str(), "r");

    if (inp == NULL) {
        printf("Could not open tag definitions input file \"%s\"\n", path.c_str());
        return -1;
    }

    unsigned long line_counter = 0;
    char buffer[1024] = { 0 };

    while (fgets(buffer, sizeof(buffer), inp) != NULL) {
        std::string line(buffer);
        line_counter++;

        // remove comments, empty lines and trailing \r or \n
        size_t hashpos = line.find_first_of("#\r\n");
        if (hashpos != std::string::npos) {
            line.erase(hashpos);
        }

        if (line.length() > 0) {
            std::string tag_id;
            std::vector<std::string> tag_info;

            // split line at first '='
            std::string::size_type pos = line.find('=');
            if (pos != std::string::npos) {
                tag_id = line.substr(0, pos);
                std::string tag_desc = line.substr(pos + 1);

                // split second part of line at \ characters
                std::string::size_type param_pos;
                while (param_pos = tag_desc.find('\\'), param_pos != std::string::npos) {
                    tag_info.push_back(tag_desc.substr(0, param_pos));
                    tag_desc = tag_desc.substr(param_pos + 1);
                }
                tag_info.push_back(tag_desc);
            }

            // create tag record
            if (tag_info.size() != 3) {
                printf("Wrong number of items in file \"%s\" at line %lu: %s\n", path.c_str(), line_counter, line.c_str());
            }
            else {
                TagRecord record;

                int n1 = sscanf(tag_id.c_str(), "%lu", &record.enum_id);
                record.tagname     = tag_info[0];
                record.description = tag_info[2];
                int n2 = sscanf(tag_info[1].c_str(), "%lu", &record.register_id);

                if (n1 == 1 && n2 == 1) {
                    add(record);
                }
            }
        }
    }

    fclose(inp);

    return (int)tagmap.size();
}


// add sbfspot tag list to global status enum id map
bool TagDefinitions::insertIntoGlobalSpeedwireStatusMap(const TagRecordMap& map) {
    SpeedwireStatusMap& status_map = SpeedwireStatusMap::getGlobalMap();
    for (const auto& def : map) {
        if (def.second.register_id == 0) {
            SpeedwireStatus status(def.second.enum_id, def.second.tagname, def.second.description);
            status_map.add(status);
        }
    }
    return true;
}


// add sbfspot tag list to global register id map
bool TagDefinitions::insertIntoGlobalSpeedwireDataMap(const TagRecordMap& map) {
    SpeedwireDataMap& data_map = SpeedwireDataMap::getGlobalMap();
    for (const auto& def : map) {
        if (def.second.register_id != 0) {
            SpeedwireData entry(Command::NONE, def.second.register_id, 0x01, SpeedwireDataType::Unsigned32, 0, NULL, 0, MeasurementType::InverterStatus(), Wire::NO_WIRE, def.second.tagname);
            data_map.add(entry);
        }
    }
    return true;
}

