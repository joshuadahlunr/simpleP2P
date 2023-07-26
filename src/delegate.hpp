#ifndef DELEGATE_HPP
#define DELEGATE_HPP

// Modified from: https://github.com/linksplatform/Delegates

#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <variant>
#include <array>
#include <span>

namespace p2p {
	template<typename... Ts>
	struct variant: public std::variant<Ts...> {
		using std::variant<Ts...>::variant;

		template <typename T>
		constexpr bool holds_type() const {
			size_t i {0};
			const auto accumulator = [&i](const bool equ) {
				i += !equ;
				return equ;
			};
			(accumulator(std::is_same_v<T, Ts>) || ...);
			return this->index() == i;
		}

		template <typename T>
		constexpr T value_or(T&& instead) const {
			return holds_type<T>() ? std::get<T>(*this) : instead;
		}
	};

	template <typename...>
	struct delegate_function;

	// Unified function type that can store functions, lambdas, methods, etc...
	template <typename ReturnType, typename... Args>
	struct delegate_function<ReturnType(Args...)> {
		using raw_function_type = ReturnType(Args...);
		using function_type = std::function<raw_function_type>;

		using argumentless_raw_function_type = ReturnType();
		using argumentless_delegate_type = delegate_function<argumentless_raw_function_type>;

		constexpr delegate_function() noexcept = default;

		constexpr delegate_function(const delegate_function&) noexcept = default;

		constexpr delegate_function(delegate_function&&) noexcept = default;

		constexpr delegate_function(raw_function_type simpleFunction) noexcept
			: function(simpleFunction) { }

		constexpr delegate_function(const function_type& complexFunction)
			: function(std::make_shared<function_type>(complexFunction)) { }

		constexpr delegate_function(const function_type&& complexFunction)
			: function(std::make_shared<function_type>(std::move(complexFunction))) { }

		constexpr delegate_function(std::shared_ptr<function_type> complexFunctionPointer)
			: function(complexFunctionPointer) { }

		template <typename Class>
		constexpr delegate_function(std::shared_ptr<Class> object, ReturnType(Class:: *member)(Args...))
			: delegate_function(std::make_shared<MemberMethod<Class>>(std::move(object), member)) { }

		// Argumentless functions
		constexpr delegate_function(const argumentless_delegate_type& argumentlessDelegateFunction) noexcept
		requires (!std::is_same_v<raw_function_type, argumentless_raw_function_type>)
			: function(std::make_shared<argumentless_delegate_type>(argumentlessDelegateFunction)) { }

		constexpr delegate_function(const argumentless_delegate_type&& argumentlessDelegateFunction) noexcept
		requires (!std::is_same_v<raw_function_type, argumentless_raw_function_type>)
			: function(std::make_shared<argumentless_delegate_type>(std::move(argumentlessDelegateFunction))) { }

		constexpr delegate_function(const std::shared_ptr<argumentless_delegate_type> argumentlessDelegateFunctionPointer) noexcept
		requires (!std::is_same_v<raw_function_type, argumentless_raw_function_type>)
			: function(argumentlessDelegateFunctionPointer) { }

		template<std::invocable<Args...> Functor>
		constexpr delegate_function(Functor&& functor)
			: function(std::make_shared<function_type>(std::move(functor))) { }

		constexpr virtual ~delegate_function() = default;

		constexpr delegate_function& operator=(const delegate_function& other) noexcept {
			if (this != &other)
				function = other.function;

			return *this;
		}

		constexpr delegate_function& operator=(delegate_function&& other) noexcept {
			if (this != &other)
				function = std::move(other.function);

			return *this;
		}

		constexpr virtual ReturnType operator()(Args... args) const {
			if(function.template holds_type<raw_function_type*>()) // SimpleFunction
				return function.template value_or<raw_function_type*>(nullptr)(std::forward<Args>(args)...);
			else if(function.template holds_type<std::shared_ptr<MemberMethodBase>>()) // MemberMethod
				return (*function.template value_or<std::shared_ptr<MemberMethodBase>>(nullptr))(std::forward<Args>(args)...);
			else if(function.template holds_type<std::shared_ptr<function_type>>()) // ComplexFunction
				return (*function.template value_or<std::shared_ptr<function_type>>(nullptr))(std::forward<Args>(args)...);
			else if constexpr(!std::is_same_v<raw_function_type, argumentless_raw_function_type>) {
				if(function.template holds_type<std::shared_ptr<argumentless_delegate_type>>()) // Argumentless Wrapper
					// TODO: gets called multiple times?
					return (*function.template value_or<std::shared_ptr<argumentless_delegate_type>>(nullptr))();
			}
			throw std::bad_function_call {};
		}

		template <typename OtherReturnType, typename... OtherArgs>
		constexpr std::strong_ordering operator<=>(const delegate_function<OtherReturnType(OtherArgs...)>& other) const {
			using Other = delegate_function<OtherReturnType(OtherArgs...)>;

			// Convert our function and the other's function to a void pointer
			void* selfPtr = nullptr, *otherPtr = nullptr;

			if(function.template holds_type<raw_function_type*>()) // SimpleFunction
				selfPtr = reinterpret_cast<void *>(function.template value_or<raw_function_type*>(nullptr));
			else if(function.template holds_type<std::shared_ptr<MemberMethodBase>>()) // MemberMethod
				selfPtr = function.template value_or<std::shared_ptr<MemberMethodBase>>(nullptr).get();
			else if(function.template holds_type<std::shared_ptr<function_type>>()) // ComplexFunction
				selfPtr = function.template value_or<std::shared_ptr<function_type>>(nullptr).get();
			else if constexpr(!std::is_same_v<raw_function_type, argumentless_raw_function_type>)
				if(function.template holds_type<std::shared_ptr<argumentless_delegate_type>>()) // Argumentless Wrapper
					return function.template value_or<std::shared_ptr<argumentless_delegate_type>>(nullptr)->operator<=>(other);

			if(other.getFunction().template holds_type<typename Other::raw_function_type*>()) // SimpleFunction
				otherPtr = reinterpret_cast<void *>(other.getFunction().template value_or<typename Other::raw_function_type*>(nullptr));
			else if(other.getFunction().template holds_type<std::shared_ptr<typename Other::MemberMethodBase>>()) // MemberMethod
				otherPtr = other.getFunction().template value_or<std::shared_ptr<typename Other::MemberMethodBase>>(nullptr).get();
			else if(other.getFunction().template holds_type<std::shared_ptr<typename Other::function_type>>()) // ComplexFunction
				otherPtr = other.getFunction().template value_or<std::shared_ptr<typename Other::function_type>>(nullptr).get();
			else if constexpr(!std::is_same_v<typename Other::raw_function_type, typename Other::argumentless_raw_function_type>)
				if(other.getFunction().template holds_type<std::shared_ptr<typename Other::argumentless_delegate_type>>()) // Argumentless Wrapper
					return this->operator<=>(*other.getFunction().template value_or<std::shared_ptr<typename Other::argumentless_delegate_type>>(nullptr));

			// Compare the void pointers
			return selfPtr <=> otherPtr;
		}

		constexpr bool operator==(const delegate_function& other) const {
			return (*this <=> other) == std::strong_ordering::equal;
		}

	//protected:
		struct MemberMethodBase {
			constexpr virtual ReturnType operator()(Args... args) const = 0;
			constexpr virtual bool operator== (const MemberMethodBase& other) const = 0;
			constexpr virtual ~MemberMethodBase() = default;
		};

	protected:
		template <typename Class>
		struct MemberMethod : public MemberMethodBase {
			constexpr MemberMethod(std::shared_ptr<Class> object, ReturnType(Class:: *method)(Args...)) noexcept
				: object(std::move(object)), method(method) { }

			constexpr ReturnType operator()(Args... args) const override {
				return (*object.*method)(std::forward<Args>(args)...);
			}

			constexpr bool operator==(const MemberMethodBase& other) const override {
				const MemberMethod *otherMethod = dynamic_cast<const MemberMethod *>(&other);
				if (!otherMethod)
					return false;

				return this->object == otherMethod->object& & this->method == otherMethod->method;
			}

		protected:
			std::shared_ptr<Class> object;
			ReturnType(Class:: *method)(Args...);
		};

		constexpr delegate_function(std::shared_ptr<MemberMethodBase> memberMethod) noexcept
			: function(std::move(memberMethod)) { }

		using variantType = typename std::conditional< std::is_same_v<raw_function_type, argumentless_raw_function_type>,
			variant<raw_function_type*, std::shared_ptr<function_type>, std::shared_ptr<MemberMethodBase>>,
			variant<raw_function_type*, std::shared_ptr<function_type>, std::shared_ptr<MemberMethodBase>, std::shared_ptr<argumentless_delegate_type>>>::type;
		variantType function;

	public:
		const variantType& getFunction() const { return function; }
	};

	// Deduction guides
	template <typename ReturnType, typename... Args>
	delegate_function(ReturnType(f)(Args...)) -> delegate_function<ReturnType(Args...)>;

	template <typename ReturnType, typename... Args>
	delegate_function(std::function<ReturnType(Args...)> f) -> delegate_function<ReturnType(Args...)>;

	template <typename Class, typename ReturnType, typename... Args>
	delegate_function(std::shared_ptr<Class> object, ReturnType(Class:: *member)(Args...)) -> delegate_function<ReturnType(Args...)>;


	// -- Make Delegate Functions --


	template <typename ReturnType, typename... Args>
	constexpr inline delegate_function<ReturnType(Args...)> make_delegate(ReturnType(func)(Args...)) { return func; }

	template <typename ReturnType, typename... Args>
	constexpr inline delegate_function<ReturnType(Args...)> make_delegate(std::function<ReturnType(Args...)>& func) { return func; }

	template <typename ReturnType, typename... Args>
	constexpr inline delegate_function<ReturnType(Args...)> make_delegate(std::function<ReturnType(Args...)>&& func) { return std::move(func); }

	// Dirrect lambda and functor support
	template <class Function>
	constexpr inline auto make_delegate(Function&& func) { return delegate_function(std::function(func)); }

	template <typename Class, typename ReturnType, typename... Args>
	constexpr inline delegate_function<ReturnType(Args...)> make_delegate(std::shared_ptr<Class> object, ReturnType(Class:: *member)(Args...)) { return {object, member}; };


	// -- Multicast Delegate --


	template <typename...>
	class delegate;

	template <typename ReturnType, typename... Args>
	class delegate<ReturnType(Args...)>: public delegate_function<ReturnType(Args...)> {
		using Base = delegate_function<ReturnType(Args...)>;
	public:
		using raw_function_type = typename Base::raw_function_type;
		using delegate_type = Base;
		using function_type = typename Base::function_type;

		// Bool indicating if the given type is a type of functor that needs to be processed differently from generic functors
		template<typename T>
		static constexpr bool is_base_functor = std::is_same_v<T, delegate_type> || std::is_same_v<T, function_type> || std::is_same_v<T, delegate>;

		constexpr delegate() noexcept = default;

		constexpr delegate(const delegate& delegate) : Base() { copyCallbacks(delegate); }
		constexpr delegate(delegate&& delegate) noexcept { moveCallbacksUnsync(std::move(delegate)); }
		constexpr delegate(const delegate_type& callback) { *this += callback; }
		constexpr delegate(const delegate_type&& callback) { *this += std::move(callback); }
		constexpr delegate(raw_function_type callback) { *this += callback; }
		constexpr delegate(const function_type& callback) { *this += callback; }
		constexpr delegate(const function_type&& callback) { *this += std::move(callback); }

		template<std::invocable<Args...> Functor> requires (!is_base_functor<Functor>)
		constexpr delegate(Functor&& functor) : delegate(std::move(delegate_type(std::move(functor)))) {}

		constexpr delegate& operator=(const delegate& other) {
			if (this !=& other)
				copyCallbacks(other);
			return *this;
		}
		constexpr inline delegate& set(const delegate& other) { return this->operator=(other); }

		constexpr delegate& operator=(delegate&& other) noexcept {
			if (this !=& other)
				moveCallbacksUnsync(std::move(other));
			return *this;
		}
		constexpr inline delegate& set(const delegate&& other) { return this->operator=(std::move(other)); }

		constexpr delegate& operator=(const delegate_type& other) {
			clearCallbacks();
			*this += other;
			return *this;
		}
		constexpr inline delegate& set(const delegate_type& other) { return this->operator=(other); }

		constexpr delegate& operator=(raw_function_type other) {
			clearCallbacks();
			*this += other;
			return *this;
		}
		constexpr inline delegate& set(raw_function_type other) { return this->operator=(other); }

		constexpr delegate& operator=(const function_type& other) {
			clearCallbacks();
			*this += other;
			return *this;
		}
		constexpr inline delegate& set(const function_type& other) { return this->operator=(other); }

		constexpr delegate& operator=(const function_type&& other) {
			clearCallbacks();
			*this += std::move(other);
			return *this;
		}
		constexpr inline delegate& set(const function_type&& other) { return this->operator=(std::move(other)); }

		template<std::invocable<Args...> Functor> requires (!is_base_functor<Functor>)
		constexpr delegate& operator=(Functor&& other) {
			clearCallbacks();
			*this += std::move(other);
			return *this;
		}
		template<std::invocable<Args...> Functor> requires (!is_base_functor<Functor>)
		constexpr inline delegate& set(Functor&& other) { return this->operator=(std::move(other)); }

		constexpr virtual delegate& operator+=(const delegate_type& callback) {
			callbacks.emplace_back(callback);
			return *this;
		}
		constexpr inline delegate& connect(const delegate_type& other) { return this->operator+=(other); }

		constexpr virtual delegate& operator+=(const delegate_type&& callback) {
			callbacks.emplace_back(callback);
			return *this;
		}
		constexpr inline delegate& connect(const delegate_type&& callback) { return this->operator+=(std::move(callback)); }

		constexpr inline delegate& operator+=(raw_function_type callback) { return *this += delegate_type {callback}; }
		constexpr inline delegate& connect(raw_function_type callback) { return this->operator+=(callback); }

		constexpr inline delegate& operator+=(const function_type& callback) { return *this += delegate_type {callback}; }
		constexpr inline delegate& connect(const function_type& callback) { return this->operator+=(callback); }

		constexpr inline delegate& operator+=(const function_type&& callback) { return *this += delegate_type {callback}; }
		constexpr inline delegate& connect(const function_type&& callback) { return this->operator+=(std::move(callback)); }

		template<std::invocable<Args...> Functor> requires (!is_base_functor<Functor>)
		constexpr inline delegate& operator+=(Functor&& callback) { return *this += delegate_type {callback}; }
		template<std::invocable<Args...> Functor> requires (!is_base_functor<Functor>)
		constexpr inline delegate& connect(Functor&& callback) { return this->operator+=(std::move(callback)); }

		constexpr virtual delegate& operator-=(const delegate_type& callback) {
			auto searchResult = std::find(callbacks.begin(), callbacks.end(), callback);
			if (searchResult != callbacks.end())
				callbacks.erase(searchResult);

			return *this;
		}
		constexpr inline delegate& disconnect(const delegate_type& callback) { return *this->operator-=(callback); }

		constexpr inline delegate& operator-=(const delegate_type&& callback) { return *this -= callback; }
		constexpr inline delegate& disconnect(const delegate_type&& callback) { return *this -= callback; }

		constexpr inline delegate& operator-=(raw_function_type& callback) { return *this -= delegate_type {callback}; }
		constexpr inline delegate& disconnect(raw_function_type& callback) { return *this->operator-=(callback); }

		constexpr inline delegate& operator-=(const function_type& callback) { return *this -= delegate_type {callback}; }
		constexpr inline delegate& disconnect(const function_type& callback) { return *this->operator-=(callback); }

		constexpr inline delegate& operator-=(const function_type&& callback) { return *this -= callback; }
		constexpr inline delegate& disconnect(const function_type&& callback) { return *this -= callback; }

		constexpr ReturnType operator()(Args... args) const override {
			if (callbacks.size() == 0)
				throw std::bad_function_call();

			for (auto callbackIt = callbacks.begin(); callbackIt != std::prev(callbacks.end()); ++callbackIt)
				(*callbackIt)(std::forward<Args>(args)...);

			return callbacks.back()(std::forward<Args>(args)...);
		}

		template<typename T>
		constexpr auto operator()(T&& returnBehavior, Args... args) const {
			if (callbacks.size() == 0)
				throw std::bad_function_call();

			using vectorType = typename std::conditional_t<std::is_reference_v<ReturnType>, std::reference_wrapper<typename std::remove_cvref<ReturnType>::type>, typename std::remove_cvref<ReturnType>::type>;
			std::vector<vectorType> results;
			results.reserve(callbacks.size());

			for (auto callbackIt = callbacks.begin(); callbackIt != callbacks.end(); ++callbackIt)
				results.emplace_back((*callbackIt)(std::forward<Args>(args)...));

			return returnBehavior(results.begin(), results.end());
		}

		constexpr auto operator<=>(const delegate& other) const { return callbacks <=> other.callbacks; }

		constexpr inline void clear() { clearCallbacks(); }

		constexpr auto size() const { return callbacks.size(); }
		constexpr auto empty() const { return callbacks.empty(); }
		constexpr operator bool() const { return !empty(); }

	protected:
		constexpr inline void moveCallbacksUnsync(delegate&& other) { callbacks = std::move(other.callbacks); }
		constexpr inline void copyCallbacksUnsync(const delegate& other) { callbacks = other.callbacks; }

		constexpr virtual void copyCallbacks(const delegate& other) {
			if (this == &other)
				return;

			copyCallbacksUnsync(other);
		}

		constexpr inline virtual void clearCallbacks() { callbacks.clear(); }

		std::vector<delegate_type> callbacks;
	};
}

#endif // DELEGATE_HPP