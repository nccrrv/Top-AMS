#pragma once
#include <thread>
#include <chrono> 
//#define NO_DEBUG

#ifndef NO_DEBUG
#include <iostream>
#endif // !NO_DEBUG



namespace mstd {

#ifdef NO_DEBUG
	template<typename T,typename ...V>
	inline constexpr void fpr(T&& a,V&&...v) {}//fpr
#else
	template<typename T>
	inline constexpr void fpr(T&& x) {
		std::cout << std::forward<T>(x) << std::endl;
	}
	template<typename T,typename ...V>
	inline constexpr void fpr(T&& a,V&&...v) {
		std::cout << std::forward<T>(a);
		fpr(std::forward<V>(v)...);
	}//fpr

	//未来可考虑扩展到web前端输出
#endif // NO_DEBUG




	template<typename T>
	inline void delay(const T& t) { std::this_thread::sleep_for(t); }


	struct call_once {
		template<typename F,typename ...V>
		call_once(const F& f,V&&...v) { f(std::forward<V>(v)...); }
	};


	template<typename T>
	void atomic_wait_un(std::atomic<T>& value,T target) {//@_@可以考虑加入mstd
		auto old_value = value.load();
		while (old_value != target) {
			value.wait(old_value);
			old_value = value;
		}
	}

}//mstd

using mstd::fpr;
using namespace std::literals::chrono_literals;
