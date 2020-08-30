#pragma once

#include <vector>   // vector
#include <memory>   // unique_ptr
#include <utility>  // as_const
#include <cassert>  // assert
#include <cstddef>  // byte

namespace ecfw 
{
namespace detail 
{

	class chunked_buffer final {

		// Owning objects who construct elements 
		// inside of a buffer are responsible for
		// destroying all of the objects they construct.
		
	public:

		chunked_buffer() = default;

		chunked_buffer(chunked_buffer&&) = default;

		chunked_buffer& operator=(chunked_buffer&&) = default;

		using destructor_fn_type = void (*) (void*);

		chunked_buffer(uint32_t object_size, // sizeof(T)
					   uint32_t chunk_size,  // # of elements per chunk
					   destructor_fn_type object_destructor) // How we'll destroy each element
			: m_blocks{}
			, m_object_destructor{object_destructor}
			, m_object_size{object_size}
			, m_chunk_size{chunk_size}
		{}
		
		// For checking if the buffer has a valid object destructor
		operator bool() const noexcept {
			return m_object_destructor != nullptr;
		}

		const void* data(uint32_t index) const {
			assert(block(index) < m_blocks.size() 
				&& m_blocks[block(index)]);
			return m_blocks[block(index)].get() + offset(index);
		}

		void* data(uint32_t index) {
			using std::as_const;
			return const_cast<void*>(as_const(*this).data(index));
		}

		template <typename T, typename... Args>
		void construct(T* p, Args&&... args) {
			// Ensure that the given object type is compatible
			// with the stored object destructor function.
			assert(&destroy_object<T> == m_object_destructor);

			// Construct the object of type T in allocated 
			// uninitialized storage pointed to by p, 
			// using placement-new
			::new ((void *)p) T(std::forward<Args>(args)...);
		}

		void accommodate(uint32_t index) {
			using std::make_unique;

			// Only allocate space for the block which includes
			// the given index. This is to reduce the amount of
			// wasted space in case only a few entities have 
			// data stored in *this.
			uint32_t block_i = block(index);
			if (block_i >= m_blocks.size())
				m_blocks.resize(block_i + 1);
			if (!m_blocks[block_i]) {
				uint32_t size_in_bytes =
					m_object_size * m_chunk_size;
				m_blocks[block_i] = 
					make_unique<std::byte[]>(size_in_bytes);
			}
		}

		void reserve(uint32_t n) {
			using std::make_unique;

			uint32_t block_n = block(n);
			if (block_n >= m_blocks.size())
				m_blocks.resize(block_n + 1);

			// Unlike accommodate(...), we need to allocate space
			// for all indices up to and including n. Therefore,
			// each block up to and including the block which 
			// includes index n must be allocated.
			uint32_t size_in_bytes = m_object_size * m_chunk_size;
			for (uint32_t block_i = 0; block_i != block_n; ++block_i) {
				// Just in case there are existing components
				if (!m_blocks[block_i])
					m_blocks[block_i] = 
						make_unique<std::byte[]>(size_in_bytes);
			}
		}

		uint32_t size() const noexcept {
			return static_cast<uint32_t>(m_blocks.size()) * m_chunk_size;
		}

		void destroy(void* p) {
			m_object_destructor(p);
		}

	private:
		
		uint32_t block(uint32_t index) const noexcept {
			return index / m_chunk_size;
		}

		uint32_t offset(uint32_t index) const noexcept {
			return (index % m_chunk_size) * m_object_size;
		}

		std::vector<std::unique_ptr<std::byte[]>> m_blocks{};
		destructor_fn_type m_object_destructor{nullptr};
		uint32_t m_object_size{0};
		uint32_t m_chunk_size{0};
	};

	template <typename T>
	void destroy_object(void* ptr) {
		static_cast<T*>(ptr)->~T();
	}

} // namespace detail
} // namespace ecfw