#ifndef _PROCMEMINFO_H
#define _PROCMEMINFO_H
#include <vector>
#include <string>
#include <memory>
#include "caps.h"
namespace rokid {
  /*
   * process memory info
   */
  class ProcMemInfo {
  private:
    uint32_t pid = 0;
    std::shared_ptr<std::string> fullName = nullptr;
    int64_t sharedClean = 0;
    int64_t sharedDirty = 0;
    int64_t privateClean = 0;
    int64_t privateDirty = 0;
    int64_t pss = 0;
    int64_t rss = 0;
  public:
    inline static std::shared_ptr<ProcMemInfo> create() {
      return std::make_shared<ProcMemInfo>();
    }
    /*
    * getter process id
    */
    inline uint32_t getPid() const {
      return pid;
    }
    /*
    * getter process command line
    */
    inline const std::shared_ptr<std::string> getFullName() const {
      return fullName;
    }
    /*
    * getter shared clean memory
    */
    inline int64_t getSharedClean() const {
      return sharedClean;
    }
    /*
    * getter shared dirty memory
    */
    inline int64_t getSharedDirty() const {
      return sharedDirty;
    }
    /*
    * getter private clean memory
    */
    inline int64_t getPrivateClean() const {
      return privateClean;
    }
    /*
    * getter private dirty memory
    */
    inline int64_t getPrivateDirty() const {
      return privateDirty;
    }
    /*
    * getter proportional set size
    */
    inline int64_t getPss() const {
      return pss;
    }
    /*
    * getter resident set size
    */
    inline int64_t getRss() const {
      return rss;
    }
    /*
    * setter process id
    */
    inline void setPid(uint32_t v) {
      pid = v;
    }
    /*
    * setter process command line
    */
    inline void setFullName(const std::shared_ptr<std::string> &v) {
      fullName = v;
    }
    /*
    * setter process command line
    */
    inline void setFullName(const char* v) {
      if (!fullName) fullName = std::make_shared<std::string>();
      *fullName = v;
    }
    /*
    * setter shared clean memory
    */
    inline void setSharedClean(int64_t v) {
      sharedClean = v;
    }
    /*
    * setter shared dirty memory
    */
    inline void setSharedDirty(int64_t v) {
      sharedDirty = v;
    }
    /*
    * setter private clean memory
    */
    inline void setPrivateClean(int64_t v) {
      privateClean = v;
    }
    /*
    * setter private dirty memory
    */
    inline void setPrivateDirty(int64_t v) {
      privateDirty = v;
    }
    /*
    * setter proportional set size
    */
    inline void setPss(int64_t v) {
      pss = v;
    }
    /*
    * setter resident set size
    */
    inline void setRss(int64_t v) {
      rss = v;
    }
    /*
     * serialize this object as buffer
    */
    int32_t serialize(void* buf, uint32_t bufsize) const;
    /*
     * deserialize this object as caps (with message type)
     */
    int32_t serialize(std::shared_ptr<Caps> &caps) const;
    /*
     * deserialize this object from buffer
     */
    int32_t deserialize(void* buf, uint32_t bufSize);
    /*
     * deserialize this object from caps (with message type)
     */
    int32_t deserialize(std::shared_ptr<Caps> &caps);
    /*
     * serialize this object as caps (without message type)
     */
    int32_t serializeForCapsObj(std::shared_ptr<Caps> &caps) const;
    /*
     * deserialize this object from caps (without message type)
     */
    int32_t deserializeForCapsObj(std::shared_ptr<Caps> &caps);
  };

}
#endif // _PROCMEMINFO_H