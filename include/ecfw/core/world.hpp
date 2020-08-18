#pragma once

#include <tuple>		  			// tuple, get, forward_as_tuple
#include <stack>		  			// stack
#include <vector>		  			// vector
#include <cassert>		  			// assert
#include <algorithm>      			// generate_n, all_of
#include <type_traits>	  			// conjunction_v, is_default_constructible, as_const, is_constructible, is_copy_constructible
#include <unordered_map>  			// unordered_map
#include <boost/hana.hpp>			// unique, equal, is_subset
#include <boost/dynamic_bitset.hpp> // dynamic_bitset
#include <ecfw/core/view.hpp>
#include <ecfw/detail/buffer.hpp>
#include <ecfw/detail/integers.hpp>
#include <ecfw/detail/sparse_set.hpp>
#include <ecfw/detail/type_index.hpp>
#include <ecfw/detail/type_traits.hpp>
#include <ecfw/detail/type_list.hpp>

namespace ecfw 
{

	/**
	 * @brief Heterogeneous structure of arrays.
	 * 
	 */
	class world final {
	public:

		/**
		 * @brief Default constructor.
		 * 
		 */
		world() = default;

		/**
		 * @brief Default move constructor.
		 * 
		 */
		world(world&&) = default;

		/**
		 * @brief Default move assignment operator.
		 * 
		 * @return *this.
		 */
		world& operator=(world&&) = default;

		/**
		 * @brief Destroys *this.
		 * 
		 */
		~world() {
			// Destroy all component data
			for (size_t type_id = 0; type_id < m_buffer_metadata.size(); ++type_id) {
				const auto& buffer_metadata = m_buffer_metadata[type_id];
				for (size_t index = buffer_metadata.find_first();
					index < buffer_metadata.size(); 
					index = buffer_metadata.find_next(index)) 
				{
					m_buffers[type_id]->destroy(index);
				}
			}
		}

		/**
		 * @brief Constructs a new entity.
		 * 
		 * @pre 
		 * Each given component type must satisfy:
		 * std::is_default_constructible_v<T> == true
		 * 
		 * @tparam Ts Components types to initialize the entity with.
		 * @return An entity identifier.
		 */
		template <typename... Ts>
		uint64_t create() {
			using std::conjunction_v;
			using std::is_default_constructible;
			using boost::hana::unique;
			using boost::hana::equal;

			// Ensure that each given component is default constructible
			static_assert(conjunction_v<is_default_constructible<Ts>...>, 
				"Assigning components through world::create requires default constructible types");

			// Check for any duplicate types
			constexpr auto type_list = dtl::type_list_v<Ts...>;
			static_assert(equal(unique(type_list), type_list), 
				"Duplicate types are not allowed!");

			uint64_t entity{};

			// There exists a reusable entity
			if (!m_free_list.empty()) {
				uint32_t idx = m_free_list.top();
				m_free_list.pop();
				uint32_t ver = m_versions[idx];
				entity = dtl::concat(ver, idx);
			}
			else {
				// Ensure it is still possible to create new entities
				uint32_t idx = static_cast<uint32_t>(m_versions.size());
				assert(idx < 0xFFFFFFFF);

				// Any new entity is effectively a new row in the component table.
				// However, we refrain from actually allocating space in the component
				// metadata and component data buffers until components are assigned.
				entity = idx;
				m_versions.push_back(0);
			}
			if constexpr (sizeof...(Ts) > 0)
				(assign<Ts>(entity), ...);
			return entity;
		}

		/**
		 * @brief Assigns each element in the range [first, first + n) an
		 * entity created by *this.
		 * 
		 * @pre 
		 * Each given component type must satisfy:
		 * std::is_default_constructible_v<T> == true.
		 * 
		 * @tparam Ts Component types to initialize each entity with.
		 * @tparam OutIt An output iterator type.
		 * @param out An output iterator.
		 * @param n The number of entities to construct.
		 */
		template <
			typename... Ts, 
			typename OutIt, 
			typename = std::enable_if_t<dtl::is_iterator_v<OutIt>>
		>
		void create_n(OutIt out, size_t n) {
			std::generate_n(out, n, [this]() mutable {
				return create<Ts...>(); 
			});
		}

		/**
		 * @brief Assigns each element in the range [first, last) an 
		 * entity created by *this.
		 * 
		 * @tparam Ts Component types to initialze each entity with.
		 * @tparam FwdIt Forward iterator type.
		 * @param first Beginning of the range of elements to generate.
		 * @param last One-past the end of the range of elements to generate.
		 */
		template <
			typename... Ts,
			typename FwdIt,
			typename = std::enable_if_t<dtl::is_iterator_v<FwdIt>>
		>
		void create(FwdIt first, FwdIt last) {
			std::generate(first, last, [this]() mutable {
				return create<Ts...>();
			});
		}

		/**
		 * @brief Constructs a new entity as a clone of another.
		 * 
		 * @pre
		 * Each given component must satisfy: 
		 * std::is_copy_constructible_v<T> == true
		 * 
		 * @tparam Ts Component types to copy.
		 * @param original The entity to copy from.
		 * @return An entity identiter.
		 */
		template <typename... Ts>
		uint64_t clone(uint64_t original) {
			using std::conjunction_v;
			using std::is_copy_constructible;
			static_assert(conjunction_v<is_copy_constructible<Ts>...>, 
				"Cloning components requires copy constructible component types.");
			uint64_t entity = create();
			(assign<Ts>(entity, get<Ts>(original)), ...);
			return entity;
		}

		/**
		 * @brief Assigns each element in the range [first, first + n) a 
		 * clone created by *this from original.
		 * 
		 * @pre
		 * Each given component must satisfy: 
		 * std::is_copy_constructible_v<T> == true
		 * 
		 * @tparam Ts Component types to copy for each entity.
		 * @tparam OutIt An output iterator type.
		 * @param original The entity to copy from.
		 * @param out An output iterator type.
		 * @param n The number of entities to construct.
		 */
		template <
			typename... Ts, 
			typename OutIt, 
			typename = std::enable_if_t<dtl::is_iterator_v<OutIt>>
		>
		void clone_n(uint64_t original, OutIt out, size_t n) {
			std::generate_n(out, n, [this, original]() mutable {
				 return clone<Ts...>(original); 
			});
		}

		/**
		 * @brief Assigns each element in the range [first, last) a
		 * clone created by *this from original
		 * 
		 * @tparam Ts Component types to copy for each entity.
		 * @tparam FwdIt Forward iterator type.
		 * @param first Beginning of the range of elements to generate.
		 * @param last One-past the end of the range of elements to generate.
		 */
		template <
			typename... Ts,
			typename FwdIt,
			typename = std::enable_if_t<dtl::is_iterator_v<FwdIt>>
		> 
		void clone(uint64_t original, FwdIt first, FwdIt last) {
			std::generate(first, last, [this, original]() mutable {
				return clone<Ts...>(original);
			});
		}

		/**
		 * @brief Checks if an entity belongs to *this.
		 * 
		 * @param eid The entity to check.
		 * @return true If the entity belongs to *this.
		 * @return false If the entity does not belong to *this.
		 */
		bool valid(uint64_t eid) const {
			uint32_t idx = dtl::lsw(eid);
			uint32_t ver = dtl::msw(eid);
			return idx < m_versions.size()
				&& m_versions[idx] == ver;
		}

		/**
		 * @brief Checks if a collection of entities belong to *this.
		 * 
		 * This member function is equivalent to the following
		 * snippet:
		 * @code
		 * for (auto entity : entities)
		 *     world.valid(entity);
		 * @endcode
		 * 
		 * @tparam InIt An input iterator type.
		 * @param first The beginning of the sequence to verify
		 * @param last One-past the end of the sequence
		 * @return true If all elements of the sequence belong to *this
		 * @return false If there exists one or more elements which do not
		 * belong to this
		 */
		template <
			typename InIt, 
			typename = std::enable_if_t<dtl::is_iterator_v<InIt>>
		>
		bool valid(InIt first, InIt last) const {
			return std::all_of(first, last, 
				[this](auto e) { 
					return valid(e); 
				}
			);
		}

		/**
		 * @brief Destroys an entity.
		 * 
		 * @warning 
		 * Upon destruction, all references to the entity's
		 * components become invalid.
		 * 
		 * @param eid The entity to destroy.
		 */
		void destroy(uint64_t eid) {
			assert(valid(eid));
			
			// Split the entity into its index and version
			uint32_t idx = dtl::lsw(eid);
			[[maybe_unused]] uint32_t ver = dtl::msw(eid);
			
			// Ensure the entity can still be reused
			assert(ver < 0xFFFFFFFF);

			// Revise the entity & make it reusable
			++m_versions[idx];
			m_free_list.push(idx);

			// Destroy all of the entity's components
			for (size_t type_id = 0;
				type_id != m_buffer_metadata.size(); ++type_id) 
			{
				if (idx < m_buffer_metadata[type_id].size()
					&& m_buffer_metadata[type_id].test(idx)) 
				{
					m_buffer_metadata[type_id].reset(idx);
					m_buffers[type_id]->destroy(idx);
				}
			}

			// Remove the entity from all groups it's a part of.
			// No need to check if the group has the entity to begin with, 
			// trying to erase a non existent element of a sparse set is a no op.
			for (auto& [_, group] : m_groups)
				group.erase(eid);
		}

		/**
		 * @brief Destroys a collection of entities.
		 * 
		 * @warning 
		 * Upon destruction, all references to the components become invalid for 
		 * each entity destroyed.
		 * 
		 * @tparam InIt An input iterator type.
		 * @param first An iterator to the beginning of a range.
		 * @param last An iterator to one-past the end of a range.
		 */
		template <
			typename InIt, 
			typename = std::enable_if_t<dtl::is_iterator_v<InIt>>
		>
		void destroy(InIt first, InIt last) {
			for (; first != last; ++first)
				destroy(*first);
		}
		
		/**
		 * @brief Checks if an entity owns a given set of components.
		 * 
		 * @tparam Ts Component types to check for.
		 * @param eid The entity to check. 
		 * @return true If the entity has each component.
		 * @return false If the entity does not have each component.
		 */
		template <
			typename... Ts, 
			typename = std::enable_if_t<sizeof...(Ts) >= 1>
		>
		bool has(uint64_t eid) const {
			using boost::hana::unique;
			using boost::hana::equal;

			constexpr auto type_list = dtl::type_list_v<Ts...>;
			static_assert(equal(unique(type_list), type_list),
				"Duplicate types are not allowed!");

			assert(valid(eid));
			if constexpr (sizeof...(Ts) == 1) {
				uint32_t idx = dtl::lsw(eid);
				size_t type_id = (dtl::type_index_v<Ts>, ...);

				return type_id < m_buffer_metadata.size()
					&& idx < m_buffer_metadata[type_id].size()
					&& m_buffer_metadata[type_id].test(idx);
			}
			else
				return (has<Ts>(eid) && ...);
		}

		/**
		 * @brief Destroys components associated with an entity.
		 * 
		 * @tparam Ts Component types to remove.
		 * @param eid The entity to remove from.
		 */
		template <
			typename... Ts,
			typename = std::enable_if_t<sizeof...(Ts) >= 1>
		>
		void remove(uint64_t eid) {
			assert(has<Ts...>(eid));

			if constexpr (sizeof...(Ts) == 1) {
				uint32_t idx = dtl::lsw(eid);
				size_t type_id = (dtl::type_index_v<Ts>, ...);

				// Remove component metadata for the entity.
				m_buffer_metadata[type_id].reset(idx);

				// Destroy the component.
				m_buffers[type_id]->destroy(idx);

				// Remove the entity from all groups which require
				// the recently removed component type
				for (auto& [filter, group] : m_groups)
					if (type_id < filter.size() && filter.test(type_id))
						group.erase(eid);
			}
			else
				(remove<Ts>(eid), ...);
		}

		/**
		 * @brief Constructs a new component associated with an entity.
		 * 
		 * @pre
		 * The given component type must satisfy the following:
		 * std::is_constructible_v<T, Args...> == true.
		 * 
		 * @tparam T Component type.
		 * @tparam Args Component constructor parameter types. 
		 * @param eid The entity the component will be associated with.
		 * @param args Component constructor parameters.
		 * @return Reference to the constructed component.
		 */
		template <
			typename T, 
			typename... Args
		>
		T& assign(uint64_t eid, Args&&... args) {
			using std::make_unique;
			using std::is_constructible_v;

			static_assert(is_constructible_v<T, Args...>, 
				"Cannot construct component type from the given arguments.");

			assert(!has<T>(eid));

			uint32_t idx = dtl::lsw(eid);
			size_t type_id = dtl::type_index_v<T>;

			// Ensure there exists buffer metadata for the component.
			if (type_id >= m_buffer_metadata.size())
				m_buffer_metadata.resize(type_id + 1);

			// Ensure there exists component metadata for the entity.
			if (idx >= m_buffer_metadata[type_id].size())
				m_buffer_metadata[type_id].resize(idx + 1);

			// Logically add the component to the entity.
			m_buffer_metadata[type_id].set(idx);

			// Ensure there exists a buffer for the component.
			if (type_id >= m_buffers.size())
				m_buffers.resize(type_id + 1);

			// Ensure there exists a data buffer for the component.
			if (!m_buffers[type_id])
				m_buffers[type_id] = make_unique<dtl::typed_buffer<T>>();

			// Ensure there physically exists memory for the new component
			m_buffers[type_id]->accommodate(idx);

			// Construct the component in memory provided by the type's buffer
			T* data = static_cast<T*>(m_buffers[type_id]->data(idx));
			::new (data) T(std::forward<Args>(args)...);

			// Add the entity to all newly applicable groups.
			// Each time an entity is assigned a new component, it must
			// be added to any existing group which shares a common set
			// of components to ensure that the applicable views
			// automatically pick up the entity.
			for (auto& [filter, group] : m_groups) {
				// Skip groups which already contain this entity.
				if (group.contains(eid))
					continue;
				// Skip groups which do not include the new component.
				if (type_id >= filter.size() || !filter.test(type_id))
					continue;

				// In order to check if an entity belongs to a group
				// We must ensure that for all active bits in the group's 
				// bitset, there exists an active component associated with
				// the entity we're trying to add.
				bool has_all = true;
				for (size_t i = filter.find_first();
					i < filter.size(); i = filter.find_next(i)) 
				{
					if (idx >= m_buffer_metadata[i].size()
						|| !m_buffer_metadata[i].test(idx)) 
					{
						has_all = false;
						break;
					}
				}
				if (has_all)
					group.insert(eid);
			}

			return *data;
		}

		/**
		 * @brief Returns an entity's components
		 * 
		 * @tparam Ts Component types to fetch.
		 * @param eid The entity to fetch for.
		 * @return Reference to a component or a tuple of references.
		 */
		template <typename... Ts>
		decltype(auto) get(uint64_t eid) {
			if constexpr (sizeof...(Ts) == 1) {
				using std::as_const;
				return (const_cast<Ts&>(as_const(*this).template get<Ts>(eid)), ...);
			}
			else {
				using std::forward_as_tuple;
				return forward_as_tuple(
					get<Ts>(eid)...
				);
			}
		}

		/*! @copydoc get */
		template <typename... Ts>
		decltype(auto) get(uint64_t eid) const {
			assert(has<Ts...>(eid));
			if constexpr (sizeof...(Ts) == 1) {
				uint32_t idx = dtl::lsw(eid);
				size_t type_id = (dtl::type_index_v<Ts>, ...);
				return (*static_cast<Ts*>(
							m_buffers[type_id]->data(idx)), ...);
			}
			else {
				using std::forward_as_tuple;
				return forward_as_tuple(
					get<Ts>(eid)...
				);
			}
		}

		/**
		 * @brief Returns the number of active entities.
		 * 
		 * @tparam Ts Components types the entities must have.
		 * @return The number of active entities.
		 */
		template <typename... Ts>
		size_t size() const {
			if constexpr (sizeof...(Ts) > 0) {
				size_t count = 0;
				uint32_t size = static_cast<uint32_t>(m_versions.size());
				for (uint32_t idx = 0; idx != size; ++idx) {
					uint64_t entity = dtl::concat(m_versions[idx], idx);
					if (has<Ts...>(entity))
						++count;
				}
				return count;
			}
			else {
				return static_cast<size_t>(m_versions.size())
					- static_cast<size_t>(m_free_list.size());
			}
		}

		/**
		 * @brief Checks if *this does not have any active entities.
		 * 
		 * @tparam Ts Component types the entities must have.
		 * @return true if there are no active entities.
		 * @return false if there area active entities.
		 */
		template <typename... Ts>
		bool empty() const {
			return size<Ts...>() == 0;
		}

		/**
		 * @brief Preallocates storage space for the given component types.
		 * 
		 * @tparam Ts Component types to reserve space for.
		 * @param n The number of components to allocated space for.
		 */
		template <
			typename... Ts,
			typename = std::enable_if_t<sizeof...(Ts) >= 1>
		>
		void reserve(size_t n) {
			if constexpr (sizeof...(Ts) == 1) {
				using std::make_unique;
				size_t type_id = (dtl::type_index_v<Ts>, ...);
				
				// Ensure there exists buffer metadata for the component type.
				if (type_id >= m_buffer_metadata.size())
					m_buffer_metadata.resize(type_id + 1);
				
				// Ensure there exists component metadata up to index n.
				if (n >= m_buffer_metadata[type_id].size())
					m_buffer_metadata[type_id].resize(n + 1);

				// Ensure there exists a buffer for the component type.
				if (type_id >= m_buffers.size())
					m_buffers.resize(type_id + 1);

				// Ensure there exists a data buffer for the component type.
				if (!m_buffers[type_id])
					m_buffers[type_id] = (make_unique<dtl::typed_buffer<Ts>>(), ...);

				// Ensure there physically exists memory for n components.
				m_buffers[type_id]->reserve(n);
			}
			else {
				(reserve<Ts>(n), ...);
			}
		}

		/**
		 * @brief Returns a view of a given set of components.
		 * 
		 * @warning
		 * A view can only be created from a unique set of 
		 * components that a world has seen before. That is,
		 * for each given component type, it has been assigned
		 * to at least one entity belonging to the constructing
		 * world.
		 * 
		 * @tparam Ts Component types to be viewed.
		 * @return An instance of ecfw::view.
		 */
		template <typename... Ts>
		ecfw::view<Ts...> view() {
			using boost::hana::equal;
			using boost::hana::unique;

			// Check for duplicate types
			constexpr auto type_list = dtl::type_list_v<Ts...>;
			static_assert(equal(unique(type_list), type_list),
				"Duplicate types are not allowed!");

			return ecfw::view<Ts...>(
				group_by<Ts...>(),
				m_buffers[dtl::type_index_v<std::decay_t<Ts>>].get()...
			);
		}

		/*! @copydoc view */
		template <typename... Ts>
		ecfw::view<Ts...> view() const {
			using boost::hana::equal;
			using boost::hana::unique;
			
			// Check for duplicate types
			constexpr auto type_list = dtl::type_list_v<Ts...>;
			static_assert(equal(unique(type_list), type_list),
				"Duplicate types are not allowed!");

			return ecfw::view<Ts...>(
				group_by<Ts...>(), 
				m_buffers[dtl::type_index_v<std::decay_t<Ts>>].get()...
			);
		}

	private:

		template <typename... Ts>
		const dtl::sparse_set& group_by() const {
			using std::max;
			using std::initializer_list;
			
			// Find the largest type id. This will the size of the filter.
			auto type_ids = { dtl::type_index_v<Ts>... };
			size_t largest_type_id = max(type_ids);

			// Ensure we're not working with any unknown components.
			// Type indices are created in sequential order upon discovery by 
			// any world. Any known component is one that has been assigned
			// to an entity is some way. Therefore, any unknown component
			// will not have buffer metadata or a data buffer associated with it
			// and cannot be viewed.
			assert(largest_type_id < m_buffer_metadata.size());

			// Build the filter in order to find an existing group or to create one.
			boost::dynamic_bitset<> filter(largest_type_id + 1);
			for (auto type_id : type_ids)
				filter.set(type_id);

			// Check if there exists a group identified by our filter.
			// If so, return it to the caller.
			auto it = m_groups.find(filter);
			if (it != m_groups.end())
				return it->second;

			// Build the initial group of entities.
			dtl::sparse_set group{};
			uint32_t size = static_cast<uint32_t>(m_versions.size());
			for (uint32_t idx = 0; idx != size; ++idx) {
				bool has_all = true;
				for (auto type_id : type_ids) {
					if (idx >= m_buffer_metadata[type_id].size() 
						|| !m_buffer_metadata[type_id].test(idx)) 
					{
						has_all = false;
						break;
					}
				}
				if (has_all)
					group.insert(dtl::concat(m_versions[idx], idx));
			}
			it = m_groups.emplace_hint(it, filter, std::move(group));
			return it->second;
		}

		// Collection of pointers to entity versions. These pointers 
		// indicate entities that can be reused when making new entities.
		std::stack<uint32_t, std::vector<uint32_t>> m_free_list{};

		// Collection of entity versions, where a version indicates how
		// many times an entity has been destroyed/reused.
		std::vector<uint32_t> m_versions{};

		// Component metadata; one bitset for each component type. A bitset is only 
		// allocated for a compoentn when it's first assigned to an entity. Moreover,
		// only enough space is allocated within the bitset to accommodate the entity 
		// to which it the component is assigned.
		std::vector<boost::dynamic_bitset<>> m_buffer_metadata{};

		// Component data. space is allocated similarly to the buffer metadata.
		std::vector<std::unique_ptr<dtl::base_buffer>> m_buffers{};

		// Filtered groups of entities. Each filter represents a common
		// set of components each of the entities in the group must possess.
		mutable std::unordered_map<boost::dynamic_bitset<>, dtl::sparse_set> m_groups{};

	};

} // namespace ecfw