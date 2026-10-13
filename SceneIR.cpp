#include "SceneIR.h"

namespace osgVerse
{

namespace
{
    // Namespace prefix so that the (per-kind) numeric ids used by osg for
    // objects / arrays / images / primitive sets never collide in the key map.
    const char* namespaceFor(const std::string& className)
    {
        if (className == "osg::Image") return "I";
        if (className.find("Array") != std::string::npos) return "A";
        if (className.find("PrimitiveSet") != std::string::npos ||
            className.find("DrawArrays") != std::string::npos ||
            className.find("DrawElements") != std::string::npos) return "P";
        return "O";
    }
}

std::string SceneIR::refKey(const std::string& className, const std::string& id)
{
    return std::string(namespaceFor(className)) + "#" + id;
}

int SceneIR::createObject(const std::string& className, const std::string& id,
                          int parent, long long offset)
{
    IRObject obj;
    obj.className = className;
    obj.id = id;
    obj.parent = parent;
    obj.offset = offset;
    obj.payload = IRObject::NONE;

    int index = (int)objects.size();
    objects.push_back(obj);
    if (!id.empty()) _keyToIndex[refKey(className, id)] = index;

    IREvent e; e.type = IREvent::CREATE_OBJECT; e.objectIndex = index;
    events.push_back(e);
    return index;
}

int SceneIR::createPayload(const IRObject& obj)
{
    int index = (int)objects.size();
    objects.push_back(obj);
    if (!obj.id.empty()) _keyToIndex[refKey(obj.className, obj.id)] = index;

    IREvent e; e.type = IREvent::CREATE_OBJECT; e.objectIndex = index;
    events.push_back(e);
    return index;
}

void SceneIR::emitCommand(int targetIndex, const std::string& name,
                          const std::vector<IRArg>& args)
{
    if (targetIndex < 0) return;
    IREvent e; e.type = IREvent::APPLY_COMMAND; e.objectIndex = targetIndex;
    e.cmd.name = name; e.cmd.args = args;
    events.push_back(e);
}

int SceneIR::resolve(const std::string& className, const std::string& id) const
{
    if (id.empty()) return -1;

    // Preferred namespace first (based on the referenced class name), then the
    // remaining ones -- the legacy compact array/primitive readers and the full
    // object readers use different class-name spellings for the same record.
    const std::string preferred = refKey(className, id);
    std::map<std::string, int>::const_iterator itr = _keyToIndex.find(preferred);
    if (itr != _keyToIndex.end()) return itr->second;

    const char* candidates[4] = { "O", "A", "I", "P" };
    for (int i = 0; i < 4; ++i)
    {
        itr = _keyToIndex.find(std::string(candidates[i]) + "#" + id);
        if (itr != _keyToIndex.end()) return itr->second;
    }
    return -1;
}

void SceneIR::clear()
{
    objects.clear();
    events.clear();
    _keyToIndex.clear();
}

void buildScene(const SceneIR& ir, ISceneBuilder& builder)
{
    for (size_t i = 0; i < ir.events.size(); ++i)
    {
        const IREvent& e = ir.events[i];
        if (e.objectIndex < 0 || e.objectIndex >= (int)ir.objects.size()) continue;
        if (e.type == IREvent::CREATE_OBJECT)
            builder.onObject(ir.objects[e.objectIndex], e.objectIndex);
        else
            builder.onCommand(ir.objects[e.objectIndex], e.objectIndex, e.cmd);
    }
    builder.finish();
}

}
