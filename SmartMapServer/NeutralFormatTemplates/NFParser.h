#pragma once
#include <boost/type_traits.hpp> // is_array, is_class, remove_bounds

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <sstream>

#include "NFExceptions.h"
#include "NFIterator.h"
#include "NFCommonDef.h"

namespace NF //neutral format
{

	namespace detail
	{
		template < typename T >
		struct array_parser
		{
			typedef array_parser< T > type;

			typedef typename boost::remove_bounds< T >::type slice_t;

			static const size_t size = sizeof(T) / sizeof(slice_t);

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, T& t)
			{
				if (boost::is_class< slice_t >::value)
				{
					Element* pChildElement = parent.GetChildElement(name, true, true);
					if (pChildElement == NULL)
						return parent;
					size_t i = 0;
					while (!pChildElement->end(name))
					{
						slice_t temp;
						NF::parser< slice_t >::parse(*pChildElement, name, temp);
						if (i >= size)
						{
							//more elements from the json array than c++ array
							throw WrongElementCountException(name);
						}
						t[i] = temp;
						i++;
					}

					if (i != size)
					{
						//more elements from c++ array than the json array
						throw WrongElementCountException(name);
					}
				}
				else
				{
					Element* pChildElement = parent.GetChildElement(name, true, false);
					if (pChildElement == NULL)
						return parent;
					size_t i = 0;
					while (!pChildElement->end(""))
					{
						slice_t temp;
						NF::parser< slice_t >::parse(*pChildElement, "", temp);
						if (i >= size)
						{
							//more elements from the json array than c++ array
							throw WrongElementCountException(name);
						}
						t[i] = temp;
						i++;
					}

					if (i != size)
					{
						//more elements from c++ array than the json array
						throw WrongElementCountException(name);
					}
				}
				return parent;
			}

		};

		template < typename T >
		struct pointer_parser
		{
			typedef pointer_parser< T > type;

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, T& t)
			{
				typedef typename boost::remove_pointer< T >::type TValType;

				try
				{
					t = new (TValType)();
					NF::parser< TValType >::parse(parent, name, *t);
				}
				catch (const MissingElementException& ex)
				{
					if (ex.name() == name)
					{
						delete t;
						t = NULL;
					}
					else
						throw;
				}
				return parent;
			}
		};

		template < typename T >
		struct struct_parser
		{
			typedef struct_parser< T > type;

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, T& t)
			{
				if (!name.empty())
				{
					Element* pChildElement = parent.GetChildElement(name, false, false);
					if (boost::is_base_of< SimpleChildElementStruct, T >::value)
						pChildElement->SetElement(SimpleChildElement, "");
					else if (boost::is_base_of< NameValueChildElementStruct, T >::value)
					{
						NameValueChildElementStruct& nameValueStruct = reinterpret_cast< NameValueChildElementStruct& >((T&)t);
						pChildElement = &pChildElement->SetElement(NameValueChildElement, nameValueStruct.itemName);
					}
					else
						pChildElement->SetElement(AttributeChild, "");
					iterator::Struct< T >::parse(*pChildElement, "", t);
				}
				else
					iterator::Struct< T >::parse(parent, "", t);
				return parent;
			}
		};


		template < typename E >
		struct struct_parser< boost::optional< E > >
		{
			typedef struct_parser< boost::optional< E > > type;

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, boost::optional< E >& t)
			{
				try
				{
					t = E();
					NF::parser< E >::parse(parent, name, t.get());
				}
				catch (const MissingElementException& ex)
				{
					if (ex.name() == name)
					{
						t.reset();
					}
					else
						throw;
				}
				return parent;
			}
		};

		template <>
		struct struct_parser< std::string >
		{
			typedef struct_parser< std::string > type;

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, std::string& t)
			{
				parent.template GetNameValueChild<std::string>(name, t);
				return parent;
			}
		};

		template <typename E>
		struct struct_parser< std::vector< E > >
		{
			typedef struct_parser< std::vector< E > > type;

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, std::vector< E >& v)
			{
				Element* pChildElement = parent.GetChildElement(name, true, true);
				if (pChildElement == NULL)
					return parent;
			
				while (!pChildElement->end(name))
				{
					E* pTemp = NULL;
					NF::parser< E* >::parse(*pChildElement, name, pTemp);
					if (pTemp != NULL)
					{
						v.push_back(*pTemp);
						delete pTemp;
					}
				}

				return parent;
			}
		};

		template < typename T >
		struct arithmetic_parser
		{
			typedef arithmetic_parser< T > type;

			template < typename Element >
			static inline Element& parse(Element& parent, const std::string& name, T& t)
			{
				parent.template GetNameValueChild< T >(name, t);
				return parent;
			}
		};

		template < typename T >
		struct calculate_parser
		{
			typedef
			typename boost::mpl::eval_if< boost::is_array< T >,
			boost::mpl::identity< array_parser < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_pointer< T >,
			boost::mpl::identity< pointer_parser < T > >,
			//else
			typename boost::mpl::eval_if< boost::is_class< T >,
			boost::mpl::identity< struct_parser < T > >,
			//else
			boost::mpl::identity< arithmetic_parser < T > >
			>
			>
			>::type type;

		};

	} // detail

	template < typename T >
	struct parser : public detail::calculate_parser < T >::type
	{
	};

	//todo: need

};// NF
