#pragma once
#include <boost/fusion/mpl.hpp>
//#include <boost/fusion/adapted.hpp> // BOOST_FUSION_ADAPT_STRUCT

// boost::fusion::result_of::value_at
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/include/value_at.hpp>

// boost::fusion::result_of::size
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/range_c.hpp>


namespace NF //neutral format
{
	template < typename T >
	struct serializer;

	template < typename T >
	struct parser;

	template < typename T >
	struct comparer;
	
	namespace detail
	{

		namespace iterator
		{


			template < typename S, typename N >
			struct StructImpl
			{

				typedef typename boost::fusion::result_of::value_at< S, N >::type current_t;
				typedef typename boost::mpl::next< N >::type next_t;
				typedef boost::fusion::extension::struct_member_name< S, N::value > name_t;
				//this is the code iterating through the member variables.
				template < typename Element >
				static inline Element& parse(Element& parent, const std::string& name, S& s)
				{
					NF::parser< current_t >::parse(parent, name_t::call(), boost::fusion::at< N >(s));

					StructImpl< S, next_t >::parse(parent, "", s);
					return parent;
				};
				template < typename Element >
				static inline Element& serialize(Element& parent, const std::string& name, const S& s)
				{
					NF::serializer< current_t >::serialize(parent, name_t::call(), boost::fusion::at< N >(s));

					StructImpl< S, next_t >::serialize(parent, "", s);
					return parent;
				}
				
				static inline bool IsEqual(const S& s1, const S& s2)
				{
					bool res = NF::comparer< current_t >::IsEqual(boost::fusion::at< N >(s1), boost::fusion::at< N >(s2));
					if (!res)
						return false;

					return StructImpl< S, next_t >::IsEqual(s1, s2);
				}
			};




			template < typename S >
			struct StructImpl< S, typename boost::fusion::result_of::size< S >::type >
			{
				template < typename Element >
				static inline Element& parse(Element& parent, const std::string& name, S& s)
				{
					return parent;
				};

				template < typename Element >
				static inline Element& serialize(Element& parent, const std::string& name, const S& s)
				{
					return parent;
				}

				static inline bool IsEqual(const S& s1, const S& s2)
				{
					return true;
				}
			};


			template < typename S >
			struct Struct : StructImpl< S, boost::mpl::int_< 0 > >
			{
			};


		} // iterator

	}//detail

}//NF