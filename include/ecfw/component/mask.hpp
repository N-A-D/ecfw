#pragma once

#include <ecfw/aliases/hana.hpp>
#include <ecfw/fwd/bitset.hpp>
#include <ecfw/utility/index_of.hpp>

namespace ecfw {
	namespace detail {

		/**
		 * @brief Encodes the first occurence of a sequence of types within a larger
		 * sequence as a bitset of the same size.
		 * 
		 * The purpose of this function is to create compile time bitsets
		 * for grouping entities together.
		 * 
		 * @pre The sequence of types to encode as 'set' must be members of the
		 * larger sequence.
		 * 
		 * @tparam Xs An iterable sequence type.
		 * @tparam Ys An iterable sequence type.
		 * @param xs A sequence of types whose indices make up bits in a bitset.
		 * @param ys A sequence of types to encode as 'set'.
		 * @return A bitset whose active bits indicates present types.
		 */
		template <typename Xs, typename Ys>
		constexpr auto make_mask(const Xs& xs, const Ys& ys) {
			auto size = decltype(hana::size(xs)){};
			bitset<size> result;
			auto encoder = [&](auto type) {
				result.set(index_of(xs, type));
			};
			hana::for_each(ys, encoder);
			return result;
		}

	}
}