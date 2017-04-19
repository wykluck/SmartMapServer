#pragma once
#include <boost/type_traits.hpp> // is_array, is_class, remove_bounds

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "NFIterator.h"
#include "math.h"

namespace NF //neutral format
{

	namespace detail
	{

		template < typename T >
		struct array_comparer
		{
			typedef array_comparer< T > type;

			typedef typename boost::remove_bounds< T >::type slice_t;

			static const size_t size = sizeof(T) / sizeof(slice_t);

			static inline bool IsEqual(const T& t1, const T& t2)
			{
				for (std::size_t i = 0; i < size; i++)
				{
					if (!NF::comparer< slice_t >::IsEqual(t1[i], t2[i]))
						return false;
				}
				
				return true;
			}

		};

		template < typename T >
		struct pointer_comparer
		{
			typedef pointer_comparer< T > type;

			static inline bool IsEqual(const T& t1, const T& t2)
			{
				typedef typename boost::remove_pointer< T >::type TValType;

				if (t1 == t2)
					return true;
				if ((t1 == NULL && t2 != NULL) || (t1 != NULL && t2 == NULL))
					return false;

				return NF::comparer< TValType >::IsEqual(*t1, *t2);
			}
		};

		template < typename T >
		struct struct_comparer
		{
			typedef struct_comparer< T > type;

			static inline bool IsEqual(const T& t1, const T& t2)
			{	
				return	iterator::Struct< T >::IsEqual(t1, t2);	
			}
		};

		template < typename E >
		struct struct_comparer< boost::optional< E > >
		{
			typedef struct_comparer< boost::optional< E > > type;

			static inline bool IsEqual(const boost::optional< E >& t1, const boost::optional< E >& t2)
			{
				if (!t1.is_initialized() && !t2.is_initialized())
					return true;
				else if ((!t1.is_initialized() && t2.is_initialized()) ||
					(t1.is_initialized() && !t2.is_initialized()))
					return false;
				else
				{
					return NF::comparer< E >::IsEqual(t1.get(), t2.get());
				}
			}
		};

		template <>
		struct struct_comparer< std::string >
		{
			typedef struct_comparer< std::string > type;

			static inline bool IsEqual(const std::string& t1, const std::string& t2)
			{
				return	t1 == t2;
			}
		};

		template < typename E >
		struct struct_comparer< std::vector< E > >
		{
			typedef struct_comparer< std::vector< E > > type;

			static inline bool IsEqual(const std::vector< E >& t1, const std::vector< E >& t2)
			{
				if (t1.size() != t2.size())
					return false;

				for (std::size_t i = 0; i < t1.size(); i++)
				{
					if (!NF::comparer< E >::IsEqual(t1[i], t2[i]))
						return false;
				}

				return true;
			}
		};

		template < typename T >
		struct floatingpoint_comparer
		{
			typedef floatingpoint_comparer< T > type;

			static inline bool IsEqual(const T& t1, const T& t2)
			{
				if (fabs(t1 - t2) < std::numeric_limits<T>::epsilon())
				{
					return true;
				}

				return false;
			}
		};

		template < typename T >
		struct default_comparer
		{
			typedef default_comparer< T > type;

			static inline bool IsEqual(const T& t1, const T& t2)
			{
				return t1 == t2;
			}
		};

		template < typename T >
		struct calculate_comparer
		{
			typedef
			typename boost::mpl::eval_if< boost::is_array< T >,
			boost::mpl::identity< array_comparer < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_pointer< T >,
			boost::mpl::identity< pointer_comparer < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_class< T >,
			boost::mpl::identity< struct_comparer < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_floating_point< T >,
			boost::mpl::identity< floatingpoint_comparer < T > >,
			//else
			boost::mpl::identity< default_comparer < T > >
			>
			>
			>
			>::type type;

		};

	} // detail

	template < typename T >
	struct comparer : public detail::calculate_comparer < T >::type
	{
	};

	//todo: need

};// NF
