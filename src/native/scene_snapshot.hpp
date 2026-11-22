#pragma once
#include "navigation_scene.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

// Development scene snapshot: one copied nav::Scene written by the
// development build ("scene dump <name>") and read back by offline fixtures.
// The body is the raw trivially copyable Scene; the header rejects a file
// written by a build with a different Scene layout. Snapshots hold game data
// and stay outside Git (%FFX_EVIDENCE_ROOT%\scenes).
namespace ffx {
struct SceneSnapshotHeader {
    char magic[8];                 // "FFXSCENE"
    std::uint32_t version;         // 1
    std::uint32_t sceneSize;       // sizeof(nav::Scene)
    std::uint32_t layout;          // sceneSnapshotLayout()
    std::uint32_t room;
    char script[12];               // identity name of part 0, or empty
    std::uint32_t scriptChecksum;  // identity checksum of part 0, or 0
    std::uint32_t bodyChecksum;    // FNV-1a of the Scene bytes
};
static_assert(sizeof(SceneSnapshotHeader)==44 && std::is_trivially_copyable_v<SceneSnapshotHeader>);
constexpr std::uint32_t SceneSnapshotVersion=1;

inline std::uint32_t sceneSnapshotFnv(const void* data,std::size_t size,std::uint32_t hash=2166136261u) {
    const auto* bytes=static_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<size;++i) {hash^=bytes[i];hash*=16777619u;}
    return hash;
}
// Offsets of the fields fixtures read. A reordered or resized member changes it.
inline std::uint32_t sceneSnapshotLayout() {
    const std::size_t offsets[]={
        offsetof(nav::Scene,epoch),offsetof(nav::Scene,count),offsetof(nav::Scene,room),offsetof(nav::Scene,player),
        offsetof(nav::Scene,controls),offsetof(nav::Scene,people),offsetof(nav::Scene,areaNames),offsetof(nav::Scene,saved),
        offsetof(nav::Scene,pedestal),offsetof(nav::Scene,parts),offsetof(nav::Scene,workers),offsetof(nav::Scene,actors),
        offsetof(nav::Scene,geometry),offsetof(nav::Scene,triangles),offsetof(nav::Scene,vertices),
        sizeof(nav::PartFact),sizeof(nav::WorkerFact),sizeof(nav::NativeGeometryContext)};
    return sceneSnapshotFnv(offsets,sizeof(offsets));
}
inline SceneSnapshotHeader sceneSnapshotHeader(const nav::Scene& snapshot,const char* script,std::uint32_t scriptChecksum) {
    SceneSnapshotHeader header{};std::memcpy(header.magic,"FFXSCENE",8);
    header.version=SceneSnapshotVersion;header.sceneSize=sizeof(nav::Scene);header.layout=sceneSnapshotLayout();
    header.room=snapshot.room;header.scriptChecksum=scriptChecksum;
    if(script) std::memcpy(header.script,script,strnlen(script,sizeof(header.script)-1));
    header.bodyChecksum=sceneSnapshotFnv(&snapshot,sizeof(snapshot));
    return header;
}
inline bool writeSceneSnapshot(const std::wstring& path,const nav::Scene& snapshot,const char* script,std::uint32_t scriptChecksum) {
    const auto header=sceneSnapshotHeader(snapshot,script,scriptChecksum);
    std::FILE* file=nullptr;if(_wfopen_s(&file,path.c_str(),L"wb") || !file) return false;
    const bool ok=std::fwrite(&header,sizeof(header),1,file)==1 && std::fwrite(&snapshot,sizeof(snapshot),1,file)==1;
    return std::fclose(file)==0 && ok;
}
// Reads a snapshot into snapshot. On failure, why names the reason.
inline bool loadSceneSnapshot(const std::wstring& path,nav::Scene& snapshot,SceneSnapshotHeader* out=nullptr,const char** why=nullptr) {
    const auto fail=[&](const char* reason) {if(why) *why=reason;return false;};
    std::FILE* file=nullptr;if(_wfopen_s(&file,path.c_str(),L"rb") || !file) return fail("cannot open the snapshot");
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> owner(file,&std::fclose);
    SceneSnapshotHeader header{};
    if(std::fread(&header,sizeof(header),1,file)!=1 || std::memcmp(header.magic,"FFXSCENE",8)) return fail("not a scene snapshot");
    if(header.version!=SceneSnapshotVersion) return fail("unsupported snapshot version");
    if(header.sceneSize!=sizeof(nav::Scene) || header.layout!=sceneSnapshotLayout()) return fail("snapshot written by a build with another Scene layout");
    if(std::fread(&snapshot,sizeof(snapshot),1,file)!=1) return fail("snapshot body is truncated");
    if(sceneSnapshotFnv(&snapshot,sizeof(snapshot))!=header.bodyChecksum) return fail("snapshot body checksum differs");
    header.script[sizeof(header.script)-1]=0;if(out) *out=header;
    return true;
}
}
