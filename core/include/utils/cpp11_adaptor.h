/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/10/30 22:58
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : cpp11_adaptor.h
 * @description : TODO
 *******************************************************/

#ifndef DEMO_TEST_CPP11_ADAPTOR_H
#define DEMO_TEST_CPP11_ADAPTOR_H

#include <memory>
#include <iterator>
#include <type_traits>

namespace Cpp11Adaptor {
		// make 智能指针
		template<typename T, typename... Args>
		std::unique_ptr<T> make_unique(Args&&... args) {
				return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
		}

		template<typename T, typename... Args>
		std::shared_ptr<T> make_shared(Args&&... args) {
				return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
		}

		// 迭代器
		template<typename Iterator>
		std::reverse_iterator<Iterator> make_reverse_iterator(Iterator it) {
				return std::reverse_iterator<Iterator>(it);
		}

		// 萃取器
		template<typename T>
		using remove_pointer_t = typename std::remove_pointer<T>::type;

		template <bool B, typename T = void>
		using enable_if_t = typename std::enable_if<B, T>::type;
}

#endif // DEMO_TEST_CPP11_ADAPTOR_H
