#pragma once
#include <boost/type_traits.hpp> // is_array, is_class, remove_bounds

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include "NFIterator.h"
#include "NFCommonDef.h"

namespace NF //neutral format
{

	namespace detail
	{

		template < typename T >
		struct array_serializer
		{
			typedef array_serializer< T > type;

			typedef typename boost::remove_bounds< T >::type slice_t;

			static const size_t size = sizeof(T) / sizeof(slice_t);

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const T& t)
			{
				if (!boost::is_class< slice_t >::value)
				{
					Element& ele = parent.AddArrayChildElement(name, SimpleElementType);
					for (std::size_t i = 0; i < size; i++)
					{
						NF::serializer< slice_t >::serialize(ele, "", t[i]);
					}
				}
				else
				{
					Element& ele = parent.AddArrayChildElement(name, ComplexElementType);
					for (std::size_t i = 0; i < size; i++)
					{
						NF::serializer< slice_t >::serialize(ele, name, t[i]);
					}
				}
				
				return parent;
			}

		};

		template < typename T >
		struct pointer_serializer
		{
			typedef pointer_serializer< T > type;

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const T& t)
			{
				typedef typename boost::remove_pointer< T >::type TValType;
				
				if (t == NULL)
					return parent;
				else
				{
					return NF::serializer< TValType >::serialize(parent, name, *t);
				}

			}
		};

		template < typename T >
		struct struct_serializer
		{
			typedef struct_serializer< T > type;

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const T& t)
			{
				if (!name.empty())
				{
					Element* pChildElement = &parent.AddChildElement(name);

					if (boost::is_base_of< SimpleChildElementStruct, T >::value)
						pChildElement->SetElement(SimpleChildElement, "");
					else if (boost::is_base_of< NameValueChildElementStruct, T>::value)
					{
						NameValueChildElementStruct& nameValueStruct = reinterpret_cast< NameValueChildElementStruct& >((T&)t);
						pChildElement = &pChildElement->SetElement(NameValueChildElement, nameValueStruct.itemName);
					}
					else
						pChildElement->SetElement(AttributeChild, "");
					iterator::Struct< T >::serialize(*pChildElement, "", t);
				}
				else
					iterator::Struct< T >::serialize(parent, "", t);
				return parent;
			}
		};


		template < typename E >
		struct struct_serializer< boost::optional< E > >
		{
			typedef struct_serializer< boost::optional< E > > type;

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const boost::optional< E >& t)
			{
				if (!t.is_initialized())
					return parent;
				else
				{
					return NF::serializer< E >::serialize(parent, name, t.get());
				}
			}
		};

		template <>
		struct struct_serializer< std::string >
		{
			typedef struct_serializer< std::string > type;

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const std::string& t)
			{
				parent.AddNameValueChild(name, t);
				return parent;
			}
		};

		template <typename E>
		struct struct_serializer< std::vector< E > >
		{
			typedef struct_serializer< std::vector< E > > type;

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const std::vector< E >& v)
			{
				if (!boost::is_class<E>::value)
				{
					Element& ele = parent.AddArrayChildElement(name, SimpleElementType);
					for (std::size_t i = 0; i < v.size(); i++)
					{
						NF::serializer< E >::serialize(ele, "", v[i]);
					}
				}
				else
				{
					Element& ele = parent.AddArrayChildElement(name, ComplexElementType);
					for (std::size_t i = 0; i < v.size(); i++)
					{
						NF::serializer< E >::serialize(ele, name, v[i]);
					}
				}
				return parent;
			}
		};

		template < typename T >
		struct arithmetic_serializer
		{
			typedef arithmetic_serializer< T > type;

			template < typename Element >
			static inline Element& serialize(Element& parent, const std::string& name, const T& t)
			{
				parent.template AddNameValueChild< T >(name, t);
				return parent;
			}
		};

		template < typename T >
		struct calculate_serializer
		{
			typedef
			typename boost::mpl::eval_if< boost::is_array< T >,
			boost::mpl::identity< array_serializer < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_pointer< T >,
			boost::mpl::identity< pointer_serializer < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_class< T >,
			boost::mpl::identity< struct_serializer < T > >,
			//else
			boost::mpl::identity< arithmetic_serializer < T > >
			>
			>
			>::type type;

		};

	} // detail

	template < typename T >
	struct serializer : public detail::calculate_serializer < T >::type
	{
	};

	//todo: need

};// NF
