#pragma once

#include "b_base.h"
#include "b_mmap.h"

#include <memory>
#include <vector>

namespace Mantids30 { namespace Memory { namespace Containers {

class B_Chunks : public B_Base
{
public:
    B_Chunks();
    B_Chunks(const std::string & str);
    ~B_Chunks() override;

    /**
     * @brief Sets the maximum memory size in bytes for a container before moves the container to disk.
     *
     * @param value Maximum memory size in bytes before data is written to disk.
     */
    void setMaxSizeInMemoryBeforeMovingToDisk(const size_t &value);

    /**
     * @brief Gets the maximum memory size in bytes for a container before moves the container to disk.
     *
     * @return Maximum memory size in bytes before data is written to disk.
     */
    size_t getMaxSizeInMemoryBeforeMovingToDisk() const;

    /**
     * @brief isUsingFiles is using mmap
     * @return true if using mmap
     */
    bool isUsingFiles();
    /**
     * @brief getMmapContainer get the container who manage the file (useful for stopping it from delete the file when finish)
     * @return pointer to the mmap container.
     */
    std::shared_ptr<B_MMAP> getMmapContainer() const;

    // Specific chunks options.
    /**
     * @brief Get Max Number of Chunks in Memory
     * @return Max Number of Chunks in Memory
     */
    size_t getMaxChunks() const;
    /**
     * @brief Set Max Number of Chunks in Memory
     * @param value Max Number of Chunks in Memory
     */
    void setMaxChunks(const size_t &value);
    /**
     * @brief setMaxChunkSize Set Maximum Chunk Size on BC_METHOD_CHUNKS
     * @param value Max Chunk size in bytes.
     */
    void setMaxChunkSize(const size_t & value);
    /**
     * @brief size Get Container Data Size in bytes
     * @return data size in bytes
     */
    virtual size_t size() override;
    /**
     * @brief findChar
     * @param c
     * @param offset
     * @return
     */
    std::optional<size_t> findChar(const int &c, const size_t &roOffset = 0, size_t  searchSpace = 0, bool caseSensitive = false) override;


protected:
    /**
     * @brief truncate the current container to n bytes.
     * @param bytes n bytes.
     * @return new container size.
     */
    std::optional<size_t> truncate2(const size_t &bytes) override;
    /**
     * @brief Append more data to current chunks. (creates new chunks of data)
     * @param data data to be appended
     * @param len data size in bytes to be appended
     * @param prependMode mode: true will prepend the data, false will append.
     * @return true if succeed
     */
    std::optional<size_t> append2(const void * buf, const size_t &len, bool prependMode = false) override;
    /**
     * @brief remove n bytes at the beggining shrinking the container
     * @param bytes bytes to be removed
     * @return bytes removed.
     */
    std::optional<size_t> displace2(const size_t &bytes = 0) override;
    /**
     * @brief free the whole container
     * @return true if succeed
     */
    bool clear2() override;
    /**
    * @brief Append this current container to a new one.
    * @param bc Binary container.
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStream2(std::ostream & bc, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) override;
    /**
    * @brief Internal Copy function to copy this container to a new one.
    * @param out data stream out
    * @param bytes size of data to be copied in bytes. -1 copy all the container but the offset.
    * @param offset displacement in bytes where the data starts.
    * @return
    */
    std::optional<size_t> copyToStreamableObject2(StreamableObject & bc, const size_t &bytes = std::numeric_limits<size_t>::max(), const size_t &offset = 0) override;
    /**
     * @brief Copy append to another binary container.
     * @param bc destination binary container
     * @param bytes size of data in bytes to be copied
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return number of bytes copied (in bytes)
     */
    std::optional<size_t> copyToBuffer2(void * buf, const size_t &bytes, const size_t &offset = 0) override;
    /**
     * @brief Compare memory with the container
     * @param mem Memory to be compared
     * @param len Memory size in bytes to be compared
     * @param offset starting point (offset) in bytes, default: 0 (start)
     * @return true where comparison returns equeal.
     */
    bool compare2(const void * buf, const size_t &len, bool caseSensitive = true, const size_t &offset = 0 ) override;

private:

    bool clearMmapedContainer();
    bool clearChunks();

    /**
     * @brief Recalculates every offset on the list
     */
    void recalcChunkOffsets();
    /**
     * @brief getChunkForOffset Get chunk containing offset
     * @param offset offset from zero on binarycontainer.
     * @return -1 if not found, or the vector position of the chunk.
     */
    size_t I_Chunk_GetPosForOffset(const size_t &offset, size_t curpos = MAX_SIZE_T, size_t curmax = MAX_SIZE_T, size_t curmin = MAX_SIZE_T);

    /**
     * @brief chunksVector defines a vector containing ordered chunks
     */
    std::vector<BinaryContainerChunk> m_chunksVector;
    /**
     * @brief Max number of Chunks in memory
     */
    size_t m_maxChunks;
    /**
     * @brief m_maxChunkSize Maximum Chunk Size.
     */
    size_t m_maxChunkSize;
    /**
     * @brief Maximum Container Size in bytes Until Swapping to FS
     */
    size_t m_maxSizeInMemoryBeforeMovingToDisk;
    /**
     * @brief mmapContainer
     */
    std::shared_ptr<B_Base> m_mmapContainer;
};

}}}

