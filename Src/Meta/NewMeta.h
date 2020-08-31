#pragma once

#include "../Misc/Callbacks.h"

class StringView;
template<std::size_t BufferSize> class AnyWithSize;
typedef AnyWithSize<8> Any;
//namespace NewMeta
namespace Meta
{
	class Object;
	class Visitor
	{
	public:
		virtual int visit(const char* name, bool&) = 0;
		virtual int visit(const char* name, int&) = 0;
		virtual int visit(const char* name, float&) = 0;
		virtual int visit(const char* name, std::string&) = 0;
		virtual int visit(const char* name, void* object, const Object&) = 0;

		virtual int visit(const char* name, bool*) = 0;
		virtual int visit(const char* name, int*) = 0;
		virtual int visit(const char* name, float*) = 0;
		virtual int visit(const char* name, std::string*) = 0;
		virtual int visit(const char* name, void** object, const Object&) = 0;
		virtual int visit(const char* name, const char*& v) { return visit(name, (std::string&)std::string(v ? v : "")); }
		virtual int visit(const char* name, StringView& v) { std::string s = v; auto r = visit(name, (std::string&)s); return r; }
		// TODO: pointer-to-array? you monster!

		virtual int startObject(const char* name, void* v, const Object& objectInfo) = 0;
		virtual int endObject() = 0;

		virtual int startArray(const char* name) = 0;
		virtual int endArray(std::size_t) = 0;

		virtual int startFunction(const char* name, bool hasReturn, bool isConstructor) = 0;
		virtual int endFunction() = 0;

		virtual int startFunctionObject(const char* name, bool hasReturn) = 0;
		virtual int endFunctionObject() = 0;

		virtual std::shared_ptr<Visitor> callbackToPreviousFunctionObject(int& id) { return false; }
		virtual int startCallback(int id) { return -1; }
		virtual int endCallback() { return -1; }
	};

	class Object
	{
	public:
		Object(StringView name);
		~Object();

		static void imgui();

		const char* getName() const;
		std::size_t getSize() const;
		void setSize(std::size_t);

		template<typename Variable, typename T> Object& var(const char* name, Variable(T::*));
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...), const std::array<const char*, sizeof...(Args)>& names, const std::vector<Any>& defaults);
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...), const std::array<const char*, sizeof...(Args)>& names);
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...));
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(*)(Args...), const std::array<const char*, sizeof...(Args)>& names, const std::vector<Any>& defaults);
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(*)(Args...), const std::array<const char*, sizeof...(Args)>& names);
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(*)(Args...));
		Object& hook(const BasicFunction<int, Visitor*, void*>&);
		Object& factory(BasicFunction<void*> constructor, BasicFunction<void, void*> destructor);
		template<typename T> Object& defaultFactory();
		template<typename T, typename... Args> Object& constructor();
		template<typename T, typename... Args> Object& constructor(const std::array<const char*, sizeof...(Args)>& names);
		template<typename T, typename... Args> Object& constructor(const std::array<const char*, sizeof...(Args)>& names, const std::vector<Any>& defaults);
		template<typename T> Object& copyOperator();

		int visit(Visitor*, void* object) const;
		template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value, int>::type = 0, typename... Args> R call(const char* name, Args...);
		template<typename T, typename R, typename std::enable_if<std::is_void<R>::value, int>::type = 0, typename... Args> void call(const char* name, Args...);
		template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value, int>::type = 0, typename... Args> R call(const char* name, T* instance, Args...);
		template<typename T, typename R, typename std::enable_if<std::is_void<R>::value, int>::type = 0, typename... Args> void call(const char* name, T* instance, Args...);
		Any callWithVisitor(const char* name, void* instance, Visitor* v);
		Any callWithVisitor(const char* name, Visitor* v);

		void* construct();
		void destruct(void*);

		void copyTo(void* dest, void* src) const;

	protected:
		std::string m_name;
		std::size_t m_size;
		std::shared_ptr<std::vector<Any>> m_members;
		void (*m_copyOperator)(void*, void*);
	};

	template<typename T>
	struct Type
	{
	public:
		static Type<T> s_instance;
	};

	class MemberVariable
	{
	public:
		virtual ~MemberVariable() =0 {};
		virtual void* get(void* instance) =0;
		virtual void* getTypeInstance() const =0;
		virtual StringView getName() const =0;
		virtual const Object* getMeta() const =0;
	};

	template<typename T, typename V>
	class MemberVariableImpl : public MemberVariable
	{
	public:
		~MemberVariableImpl();
		void* get(void* instance);
		void* getTypeInstance() const;
		StringView getName() const;
		const Object* getMeta() const;

		std::string m_name;
		V (T::* m_ptr);
	};

	class Function
	{
	public:
		virtual ~Function() = 0 { };
		virtual Any callWithVisitor(void* instance, Visitor* v);
		virtual Any callWithVisitor(Visitor* v);
		virtual int visit(Visitor*) =0;
		virtual void* getTypeInstance() const =0;

		std::string m_name;
		std::vector<const char*> m_names;
		std::vector<Any> m_defaults;
		bool m_isConstructor;
	};

	template<typename T, typename R, typename... Args>
	class Method : public Function
	{
	public:
		int visit(Visitor*);
		void* getTypeInstance() const;

		R(T::*m_ptr)(Args...);
	};

	template<typename T, typename R, typename... Args> 
	class MethodCallable : public Method<T, R, Args...>
	{
	public:
		Any callWithVisitor(void* instance, Visitor* v);
	};

	template<typename T, typename... Args>
	class MethodCallable<T, void, Args...> : public Method<T, void, Args...>
	{
	public:
		Any callWithVisitor(void* instance, Visitor* v);
	};

	template<typename T, typename R, typename... Args>
	class StaticFunction : public Function
	{
	public:
		Any callWithVisitor(Visitor* v);
		int visit(Visitor*);
		void* getTypeInstance() const;

		R(*m_ptr);
	};

	class Hook
	{
	public:

	};

	class CallFailure { };

	template<typename T> Object instanceMeta() noexcept;
	template<typename T> Object& getMeta();
	template<typename T> constexpr bool hasMeta();
	template<typename T> Object* getMetaIfAvailable();
	template<typename Object> int visit(Visitor* v, const char* name, Object* o);

	// ----------------------- IMPLEMENTATION -----------------------
	template<> Object Meta::instanceMeta<glm::vec4>();
	template<> Object Meta::instanceMeta<glm::vec3>();
	template<> Object Meta::instanceMeta<glm::vec2>();
	template<> Object Meta::instanceMeta<glm::quat>();
	template<typename T> Type<T> Type<T>::s_instance;

	template<typename Variable, typename T> Object& Object::var(const char* name, Variable(T::*v))
	{
		auto impl = new MemberVariableImpl<T, Variable>();
		impl->m_name = name;
		impl->m_ptr = v;
		m_members->emplace_back(static_cast<MemberVariable*>(impl));
		return *this;
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>& names, const std::vector<Any>& defaults)
	{
		auto impl = new MethodCallable<T, R, Args...>();
		impl->m_name = name;
		impl->m_ptr = f;
		impl->m_names.insert(impl->m_names.end(), names.begin(), names.end());
		impl->m_defaults = defaults;
		m_members->emplace_back(static_cast<Function*>(impl));
		return *this;
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>& names)
	{
		return func(name, f, names, {});
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(T::* f)(Args...))
	{
		return func(name, f, {}, {});
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(*f)(Args...), const std::array<const char*, sizeof...(Args)>& names, const std::vector<Any>& defaults)
	{
		auto impl = new StaticFunction<T, R, Args...>();
		impl->m_name = name;
		impl->m_ptr = f;
		impl->m_names = names;
		impl->m_defaults = defaults;
		impl->m_isConstructor = false;
		m_members.emplace_back(static_cast<Function*>(impl));
		return *this;
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(*f)(Args...), const std::array<const char*, sizeof...(Args)>& names)
	{
		return func(name, f, names, {});
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(*f)(Args...))
	{
		return func(name, f, {}, {});
	}

	template<typename T> Object& Object::defaultFactory()
	{
		return factory([](void*) -> void* { return new T; }, [](void* ud, void* v) { delete* (T**)v; });
	}

	template<typename T, typename... Args> Object& Object::constructor()
	{
		return constructor<T, Args...>({}, {});
	}

	template<typename T, typename... Args> Object& Object::constructor(const std::array<const char*, sizeof...(Args)>& names)
	{
		return constructor<T, Args...>(names, {});
	}

	template<typename T, typename... Args> Object& Object::constructor(const std::array<const char*, sizeof...(Args)>& names, const std::vector<Any>& defaults)
	{
		T* (*f)(Args&&...) = [](Args&&... args) { return new T(std::forward<Args>(args)...); };

		auto impl = new StaticFunction<T, void, Args...>();
		impl->m_name = "";
		impl->m_ptr = f;
		impl->m_names.insert(impl->m_names.end(), names.begin(), names.end());
		impl->m_defaults = defaults;
		impl->m_isConstructor = true;
		m_members->emplace_back(static_cast<Function*>(impl));
		return *this;
	}

	template<typename T> Object& Object::copyOperator()
	{
		m_copyOperator = [](void* dest, void* src) {
			T* d = static_cast<T*>(dest);
			T* s = static_cast<T*>(src);
			*d = *s;
		};
		return *this;
	}

	template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value, int>::type, typename... Args> R Object::call(const char* name, Args...)
	{
		for (Any& any : m_members)
		{
			if (!any.isType<Function*>())
				continue;

			Function* f = any.get<Function*>();
			if (strcmp(f->getName(), name) == 0)
			{
				if (f->getTypeInstance() == StaticFunction<T, R, Args...>::s_typeInstance)
				{
					HERE;
					return R{};
				}
			}
		}

		LOG_F(FATAL, "Failed to find function %s\n", name);
		return R{};
	}

	template<typename T, typename R, typename std::enable_if<std::is_void<R>::value, int>::type, typename... Args> void Object::call(const char* name, Args... args)
	{
		for (Any& any : m_members)
		{
			if (!any.isType<Function*>())
				continue;

			Function* f = any.get<Function*>();
			if (strcmp(f->getName(), name) == 0)
			{
				if (f->getTypeInstance() == StaticFunction<T, R, Args...>::s_typeInstance)
				{
					std::tuple<Args...> t(args...);
					HERE;
				}
			}
		}

		LOG_F(FATAL, "Failed to find function %s\n", name);
	}

	template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value, int>::type, typename... Args> R Object::call(const char* name, T* instance, Args...)
	{
		for (Any& any : m_members)
		{
			if (!any.isType<Function*>())
				continue;

			Function* f = any.get<Function*>();
			if (strcmp(f->getName(), name) == 0)
			{
				if (f->getTypeInstance() == StaticFunction<T, R, Args...>::s_typeInstance)
				{
					HERE;
					return R{};
				}
			}
		}

		LOG_F(FATAL, "Failed to find function %s\n", name);
		return R{};
	}

	template<typename T, typename R, typename std::enable_if<std::is_void<R>::value, int>::type, typename... Args> void Object::call(const char* name, T* instance, Args...)
	{
		for (Any& any : m_members)
		{
			if (!any.isType<Function*>())
				continue;

			Function* f = any.get<Function*>();
			if (strcmp(f->getName(), name) == 0)
			{
				if (f->getTypeInstance() == Method<T, R, Args...>::s_typeInstance)
				{
					std::tuple<Args...> t(args...);
					HERE;
				}
			}
		}

		LOG_F(FATAL, "Failed to find function %s\n", name);
	}

	template<typename T, typename V> MemberVariableImpl<T, V>::~MemberVariableImpl() {}

	template<typename T, typename V>
	void* MemberVariableImpl<T, V>::get(void* instance)
	{
		return &(((T*)instance)->*m_ptr);
	}

	template<typename T, typename V>
	void* MemberVariableImpl<T, V>::getTypeInstance() const
	{
		return &Type<V>::s_instance;
	}

	template<typename T, typename V>
	StringView MemberVariableImpl<T, V>::getName() const
	{
		return m_name;
	}

	template<typename T, typename V>
	const Object* MemberVariableImpl<T, V>::getMeta() const
	{
		return Meta::getMetaIfAvailable<V>();
	}

	template<typename T, typename R, typename... Args>
	Any MethodCallable<T, R, Args...>::callWithVisitor(void* instance, Visitor* v)
	{
		T* c = static_cast<T*>(instance);
		std::tuple<typename std::decay<Args>::type...> args;
		TypeVisitor<Args...>::visit(v, args, &m_defaults);
		return callWithTuple(instance, m_ptr, args);
	}

	template<typename T, typename... Args>
	Any MethodCallable<T, void, Args...>::callWithVisitor(void* instance, Visitor* v)
	{
		T* c = static_cast<T*>(instance);
		std::tuple<typename std::decay<Args>::type...> args;
		TypeVisitor<Args...>::visit(v, args, &m_defaults);
		callWithTuple(instance, m_ptr, args);
		return nullptr;
	}

	template<typename... Ts> struct TypeVisitor;
	template<typename T, typename... Ts> struct TypeVisitor<T, Ts...> { 
		static int visit(Visitor* v, const char** argNames, Any* defaults, std::size_t defaultCount);
		template<typename Tuple> static int visit(Visitor*, Tuple& t, const std::vector<Any>* defaults = nullptr);
	};
	template<> struct TypeVisitor<void> {
		static int visit(Visitor* v, const char** argNames, Any* defaults, std::size_t defaultCount) { return 0; } template<typename Tuple> static int visit(Visitor*, Tuple&, const std::vector<Any>* = nullptr) { return 0; }
	};
	template<> struct TypeVisitor<> {
		static int visit(Visitor* v, const char** argNames, Any* defaults, std::size_t defaultCount) { return 0; } template<typename Tuple> static int visit(Visitor*, Tuple&, const std::vector<Any>* = nullptr) { return 0; }
	};

	template<typename T, int type> struct TypeVisitorSpecial;
	template<typename T> constexpr bool TTypeException() { return std::is_same<StringView, T>::value || std::is_same<std::string, T>::value; }
	template<typename T> constexpr int TType() { if (TTypeException<T>()) return 2; else if (std::is_class<std::remove_reference<T>::type>::value) return 0; else if (std::is_class<std::remove_pointer<T>::type>::value) return 1; else return 2; }
	template<typename T> struct TypeVisitorSpecial<T, 0> { static int visit(Visitor* v, T& value, const char* name) { return v->visit(name, (void*)&value, getMeta<std::decay<T>::type>()); } };
	template<typename T> struct TypeVisitorSpecial<T, 1> { static int visit(Visitor* v, T& value, const char* name) { return v->visit(name, value, getMeta<std::remove_pointer<T>::type>()); } };
	template<typename T> struct TypeVisitorSpecial<T, 2> { static int visit(Visitor* v, T& value, const char* name) { return v->visit(name, value); } };
	template<> struct TypeVisitorSpecial<void, 2> { static int visit(Visitor* v, void* value, const char* name) { return 0; } };

	template<typename T, typename... Ts>
	int TypeVisitor<T, Ts...>::visit(Visitor* v, const char** argNames, Any* defaults, std::size_t defaultCount)
	{
		int r = 0;
		if (sizeof...(Ts) < defaultCount)
		{
			Any& t = defaults[defaultCount - (sizeof...(Ts) + 1)];
			r = TypeVisitorSpecial<T, TType<T>()>::visit(v, t.get<std::decay<T>::type>(), argNames ? *argNames : nullptr);
			//r = v->visit(argNames ? *argNames : nullptr, t.get<T>());
		}
		else
		{
			T t{};
			r = TypeVisitorSpecial<T, TType<T>()>::visit(v, t, argNames ? *argNames : nullptr);
			//r = v->visit(argNames ? *argNames : nullptr, t);
		}
		return r == 0 ? TypeVisitor<Ts...>::visit(v, argNames ? ++argNames : nullptr, defaults, defaultCount) : r;
	}

	template<typename T, typename... Ts>
	template<typename Tuple>
	int TypeVisitor<T, Ts...>::visit(Visitor* v, Tuple& t, const std::vector<Any>* defaults)
	{
		const std::size_t tupleIndex = std::tuple_size<Tuple>::value - (sizeof...(Ts) + 1);
		int r = TypeVisitorSpecial<T, TType<T>()>::visit(v, std::get<tupleIndex>(t), nullptr);
		if (r != 0)
		{
			if (!defaults)
				return r;

			const std::size_t defaultsIndex = (std::tuple_size<Tuple>::value - 1) - tupleIndex;
			if (defaultsIndex < defaults->size())
			{
				CHECK_F(defaults->at(defaultsIndex).isType<std::decay<T>::type>());
				std::get<tupleIndex>(t) = defaults->at(defaultsIndex).get<std::decay<T>::type>();
			}
			else
				return r;
		}
		return TypeVisitor<Ts...>::visit(v, t, defaults);
	}

	template<typename R, typename... Args> struct TypeVisitorSpecial<std::function<R(Args...)>, 0> {
		static int visit(Visitor* v, std::function < R(Args...)>& value, const char* name) {
			if (v->startFunctionObject(name, !std::is_void<R>::value) == 0)
			{
				if (!std::is_void<R>::value)
				{
					const char* name = "return";
					TypeVisitorSpecial<R, TType<R>()>::visit(v, nullptr, name);
				}

				TypeVisitor<Args...>::visit(v, nullptr, nullptr, 0);
				v->endFunctionObject();
			}

			int funcId = -std::numeric_limits<int>::max();
			std::shared_ptr<Visitor> callback = v->callbackToPreviousFunctionObject(funcId);
			if (callback)
			{
				// TODO: specialize when R != void
				value = [funcId, callback](Args... args) -> void
				{
					std::tuple<Args...> t(args...);
					if (callback->startCallback(funcId) == 0)
					{
						TypeVisitor<Args...>::visit(callback.get(), t);
						callback->endCallback();
					}
				};
			}
			return 0;
		}
	};

	template<typename T, typename R, typename... Args>
	int Method<T, R, Args...>::visit(Visitor* v)
	{
		int r = v->startFunction(m_name.c_str(), std::is_void<R>::value, false);
		if (r == 0)
		{
			if (!std::is_void<R>::value)
			{
				const char* name = "return";
				TypeVisitor<R>::visit(v, &name, nullptr, 0);
			}

			TypeVisitor<Args...>::visit(v, sizeof...(Args) == 0 ? nullptr : &m_names[0], nullptr, 0);
			v->endFunction();
		}

		return r;
	}

	template<typename T, typename R, typename... Args>
	void* Method<T, R, Args...>::getTypeInstance() const
	{
		return &Type<R(Args...)>::s_instance;
	}

	template<typename T, typename R, typename... Args>
	Any StaticFunction<T, R, Args...>::callWithVisitor(Visitor* v)
	{
		return nullptr;
	}

	template<typename T, typename R, typename... Args>
	int StaticFunction<T, R, Args...>::visit(Visitor*)
	{
		return 0;
	}

	template<typename T, typename R, typename... Args>
	void* StaticFunction<T, R, Args...>::getTypeInstance() const
	{
		return &Type<R(T::*)(Args...)>::s_instance;
	}

	template<typename T>
	Object instanceMeta() noexcept
	{
		static_assert(false, "No meta object setup");
		return {};
	}

	template<typename T>
	constexpr bool hasMeta()
	{
		// we're trying to figure out if instanceMeta is specialized without evaluating it and triggering the static_assert, specializations shouldn't have a noexcept modifier on them so let's exploit that
		return !noexcept(instanceMeta<T>());
	}

	template<typename T, bool> struct GetMetaIfAvailable {};
	template<typename T> struct GetMetaIfAvailable<T, true> { static Object* get() { return &getMeta<T>(); } };
	template<typename T> struct GetMetaIfAvailable<T, false> { static Object* get() { return nullptr; } };

	template<typename T>
	Object* getMetaIfAvailable()
	{
		return GetMetaIfAvailable<T, hasMeta<T>()>::get();
	}

	extern std::vector<Object*> g_allMetaObjects;
	template<typename T>
	Object& getMeta()
	{
		auto init = []()
		{
			Object o = instanceMeta<T>();
			o.setSize(sizeof(T));
			return o;
		};
		auto reg = [](Object& o) { g_allMetaObjects.push_back(&o); return 0; };
		static Object o = init();
		static int i = reg(o);
		return o;
	}

	template<typename Object>
	int visit(Visitor* v, const char* name, Object* instance)
	{
		Meta::Object& o = getMeta<Object>();
		int result = 0;
		if (v->startObject(name, instance, o) == 0)
		{
			result = o.visit(v, instance);
			v->endObject();
		}
		return result;
	}
}