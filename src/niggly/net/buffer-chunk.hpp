
#pragma once

namespace niggly::net
{

/*
struct BufferChunk
{
   void* data        = nullptr;
   uint32_t size     = 0;
   uint32_t capacity = 0;

   BufferChunk() = default;
   BufferChunk(uint32_t capacity_)
       : data(std::malloc(capacity_))
       , size{0}
       , capacity{capacity_}
   {}
   BufferChunk(const BufferChunk&) = delete;
   BufferChunk(BufferChunk&& o) noexcept { *this = std::move(o); }
   ~BufferChunk() { std::free(data); }
   BufferChunk& operator=(const BufferChunk&) = delete;
   BufferChunk& operator=(BufferChunk&& o) noexcept { swap(*this, o); }
   void swap(BufferChunk& o)
   {
      using std::swap;
      swap(data, o.data);
      swap(size, o.size);
      swap(capacity, o.capacity);
   }
   friend void swap(BufferChunk& a, BufferChunk& b) noexcept { a.swap(b); }

   bool is_empty() const noexcept { return size == 0; }
   bool is_full() const noexcept { return size == capacity; }
   void push_back(char value) noexcept
   {
      assert(size < capacity);
      data[size++] = value;
   }
};

template<uint32_t k_chunk_size> class WebsocketBuffer
{
 private:
   std::deque<BufferChunk> chunks_;

   BufferChunk& current_()
   {
      if(chunks_.size() == 0) chunks_.emplace_back(k_chunk_size);
      return chunks_.back();
   }

   BufferChunk& next_()
   {
      if(chunks_.size() == 0 || chunks_.back().is_full()) chunks_.emplace_back(k_chunk_size);
      return chunks_.back();
   }

 public:
   WebsocketBuffer()                                      = default;
   WebsocketBuffer(const WebsocketBuffer&)                = delete;
   WebsocketBuffer(WebsocketBuffer&&) noexcept            = default;
   ~WebsocketBuffer()                                     = default;
   WebsocketBuffer& operator=(const WebsocketBuffer&)     = delete;
   WebsocketBuffer& operator=(WebsocketBuffer&&) noexcept = default;

   void reset() { chunks_.clear(); }
   void push_back(char value) { next_().push_back(value); }
};
*/

}
