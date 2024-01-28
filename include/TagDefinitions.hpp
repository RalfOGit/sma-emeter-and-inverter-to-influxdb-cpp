#ifndef __TAGDEFINITIONS_HPP__
#define __TAGDEFINITIONS_HPP__

#include <cstdint>
#include <map>
#include <string>


class TagDefinitions
{
public:

    class TagRecord {
    public:
        uint32_t    enum_id;        // status / enum id
        uint32_t    register_id;    // register id
        std::string tagname;        // tag short name
        std::string description;    // tag description

        TagRecord() : enum_id(0), register_id(0) {}
        TagRecord(uint32_t tagID, uint32_t registerID, const std::string& _tag, const std::string& desc) : enum_id(tagID), register_id(registerID), tagname(_tag), description(desc) {}
    };

    typedef std::map<uint32_t, TagRecord> TagRecordMap;

protected:
    TagRecordMap tagmap;
    void add(const TagRecord& record) {
        tagmap[record.enum_id] = record;
    }

public:
    TagDefinitions() {}

    int readFromFile(const std::string& path);

    const std::map<uint32_t, TagRecord>& getTagMap(void) const {
        return tagmap;
    }

    static bool insertIntoGlobalSpeedwireStatusMap(const TagRecordMap& map);
    static bool insertIntoGlobalSpeedwireDataMap(const TagRecordMap& map);
};

#endif
