#pragma once
#include <array>
#include <initializer_list>
#include "../Misc/ResizableMemoryPool.h"
#include "../Misc/Misc.h"
#include "../Misc/Callbacks.h"
#include "../Misc/Any.h"

namespace Meta
{
	class Object;
	template<typename T, bool = std::is_fundamental<T>::value> struct PointerType {};
	template<typename T> struct PointerType<T, true> { typedef int Fundamental; };
	template<typename T> struct PointerType<T, false> { typedef int Object; };
	template<typename T, bool = std::is_class<T>::value, bool = std::is_fundamental<T>::value, bool = std::is_pointer<T>::value> struct MetaType { struct PointerType {}; };
	template<typename T> struct MetaType<T, true, false, false> { typedef int Class; struct PointerType {}; };
	template<typename T> struct MetaType<T, false, false, true> { typedef PointerType< typename std::remove_pointer<T>::type > PointerType; };
	template<> struct MetaType<std::string, true, false, false> { typedef int Fundamental; struct PointerType {}; };
	template<typename T> struct MetaType<T, false, true, false> { typedef int Fundamental; struct PointerType {}; };
	template<typename T, typename U> struct MetaType<std::vector<T, U>, true, false, false> { typedef int Array; struct PointerType {}; };
	template<typename T, std::size_t N> struct MetaType<std::array<T, N>, true, false, false> { typedef int Array; struct PointerType {}; };
	template<typename K, typename V> struct MetaType<std::map<K, V>, true, false, false> { typedef int Array; struct PointerType {}; };

	class Visitor
	{
	public:
		virtual int visit(const char* name, bool&) = 0;
		virtual int visit(const char* name, int&) = 0;
		virtual int visit(const char* name, float&) = 0;
		virtual int visit(const char* name, std::string&) = 0;

		virtual int visit(const char* name, bool*) = 0;
		virtual int visit(const char* name, int*) = 0;
		virtual int visit(const char* name, float*) = 0;
		virtual int visit(const char* name, std::string*) = 0;
		virtual int visit(const char* name, void* object, const Object&) =0;
		// TODO: pointer-to-array? you monster!

		virtual int startObject(const char* name, const Meta::Object& objectInfo) = 0;
		virtual int endObject() = 0;

		virtual int startArray(const char* name) = 0;
		virtual int endArray(std::size_t) = 0;

		virtual int startFunction(const char* name, bool hasReturn) = 0;
		virtual int endFunction() = 0;
	};

	class Base : public VariableSizedMemoryPool<Base>::Element
	{
	public:
		enum class BaseType { Object, Variable, Function, Hook } m_baseType;
		Base(std::size_t, BaseType type);
	};

	class Object : public Base
	{
	public:
		Object(const char* name);
		~Object();

		const char* getName() const;

		template<typename Variable, typename T> Object& var(const char* name, Variable(T::*));
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...), const std::array<const char*, sizeof...(Args)>& names, const std::array<Any, sizeof...(Args)>& defaults);
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...), const std::array<const char*, sizeof...(Args)>& names);
		template<typename T, typename R, typename... Args> Object& func(const char* name, R(T::*)(Args...));
		Object& hook(const BasicFunction<int, Visitor*, void*>&);

		int visit(Visitor*, void* object);
		template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value, int>::type = 0, typename... Args>
		R call(const char* name, T* instance, Args...);
		template<typename T, typename R, typename std::enable_if<std::is_void<R>::value, int>::type = 0, typename... Args>
		void call(const char* name, T* instance, Args...);

	protected:
		const char* m_name;
		VariableSizedMemoryPool<Base, PODPoolType<Base>> m_variables;
	};

	class Variable : public Base
	{
	protected:
		static const std::size_t MemberPointerSize = sizeof(int(Variable::*));

	public:
		template<typename ClassType, typename VariableType> Variable(const char* name, VariableType (ClassType::*Pointer));
		int visit(Visitor*, void* object);

	protected:
		template<typename ClassType, typename VariableType, typename MetaType<VariableType>::Array = 0>
		static int visitImplementation(Visitor*, const char* name, void* ptr, void* obj);
		template<typename ClassType, typename VariableType, typename MetaType<VariableType>::Fundamental = 0>
		static int visitImplementation(Visitor*, const char* name, void* ptr, void* obj);
		template<typename ClassType, typename VariableType, typename MetaType<VariableType>::Class = 0>
		static int visitImplementation(Visitor*, const char* name, void* ptr, void* obj);
		template<typename ClassType, typename VariableType, typename MetaType<VariableType>::PointerType::Fundamental = 0>
		static int visitImplementation(Visitor*, const char* name, void* ptr, void* obj);
		template<typename ClassType, typename VariableType, typename MetaType<VariableType>::PointerType::Object = 0>
		static int visitImplementation(Visitor*, const char* name, void* ptr, void* obj);
		
	protected:
		char m_pointerBuffer[MemberPointerSize];
		int(*m_visitFunction)(Visitor*, const char* name, void* ptr, void* obj);
		const char* m_name;
	};

	class Function : public Base
	{
	protected:
		static const std::size_t MemberPointerSize = sizeof(void(Function::*)());

		struct TypeBase
		{
			virtual int visit(Visitor*, const char* name, void* object, const char** names, Any* defaults) =0;
		};

		template<typename ClassType, typename R, typename... Args>
		struct TypeImplementation : public TypeBase
		{
			typedef R(ClassType::*FuncPtr)(Args...);
			static TypeImplementation<ClassType, R, Args...> Instance;
			R call(ClassType* object, void* ptr, Args...);
			int visit(Visitor*, const char* name, void* object, const char** names, Any* defaults);
		};

		template<typename... Ts>				struct TypeVisitor;
		template<typename T, typename... Ts>	struct TypeVisitor<T, Ts...> { static int visit(Visitor*, const char** argNames, Any* defaults); };
		template<>								struct TypeVisitor<> { static int visit(Visitor*, const char** argNames, Any* defaults); };

	public:
		template<typename T, typename R, typename... Args>
		static std::size_t sizeNeeded(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>* names, const std::array<Any, sizeof...(Args)>* defaults);
		template<typename T, typename R, typename... Args> Function(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>* names, const std::array<Any, sizeof...(Args)>*);
		~Function();

		int visit(Visitor*, void* object);
		template<typename Object, typename R, typename... Args>
		R call(Object* object, Args...);

		const char* getName() const;

	protected:
		char m_pointerBuffer[MemberPointerSize];
		TypeBase* m_implementation;
		const char* m_name;
		std::size_t m_argCount;
		std::size_t m_defaultCount;
		// (arg names and defaults are stored after this class)
	};
	
	class StaticFunction : public Base
	{
	protected:
		static const std::size_t MemberPointerSize = sizeof(void(*)());

	public:
		

	protected:
		char m_pointerBuffer[MemberPointerSize];
		const char* m_name;
		std::size_t m_argCount;
		std::size_t m_defaultCount;
	};

	class Hook : public Base
	{
	public:
		Hook(const BasicFunction<int, Visitor*, void*>&);
		~Hook();

		int visit(Visitor*, void* object);

	protected:
		BasicFunction<int, Visitor*, void*> m_fn;
	};

	template<typename ClassType, typename R, typename... Args>
	Function::TypeImplementation<ClassType, R, Args...> Function::TypeImplementation<ClassType, R, Args...>::Instance;

	template<typename T> Object getMeta();
	template<typename T> Object& getMetaSingleton();
	template<typename Object> int visit(Visitor* v, const char* name, Object* o);

	void test();

	// ----------------------- IMPLEMENTATION -----------------------
	template<typename Variable, typename T>
	Object& Object::var(const char* name, Variable(T::*v))
	{
		m_variables.emplace_back<Meta::Variable>(name, v);
		return *this;
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>& names, const std::array<Any, sizeof...(Args)>& defaults)
	{
		m_variables.emplace_back_with_size<Meta::Function>(Function::sizeNeeded(name, f, &names, &defaults), name, f, &names, &defaults);
		return *this;
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>& names)
	{
		m_variables.emplace_back_with_size<Meta::Function>(Function::sizeNeeded(name, f, &names, nullptr), name, f, &names, nullptr);
		return *this;
	}

	template<typename T, typename R, typename... Args> Object& Object::func(const char* name, R(T::*f)(Args...))
	{
		m_variables.emplace_back_with_size<Meta::Function>(Function::sizeNeeded(name, f, nullptr, nullptr), name, f, nullptr, nullptr);
		return *this;
	}

	template<typename T, typename R, typename std::enable_if<std::is_void<R>::value, int>::type, typename... Args>
	void Object::call(const char* name, T* instance, Args... args)
	{
		for (Base& base : m_variables)
		{
			if (base.m_baseType == BaseType::Function && strcmp(static_cast<Function&>(base).getName(), name) == 0)
			{
				static_cast<Function&>(base).call<T, R, Args...>(instance, std::forward<Args>(args)...);
				return;
			}
		}

		LOG_F(FATAL, "Failed to find function %s\n", name);
	}

	template<typename T, typename R, typename std::enable_if<!std::is_void<R>::value, int>::type, typename... Args>
	R Object::call(const char* name, T* instance, Args... args)
	{
		for (Base& base : m_variables)
		{
			if (base.m_baseType == BaseType::Function && strcmp(static_cast<Function&>(base).getName(), name) == 0)
			{
				return static_cast<Function&>(base).call<T, R, Args...>(instance, std::forward<Args>(args)...);
			}
		}

		LOG_F(FATAL, "Failed to find function %s\n", name);
		return R{};
	}

	template<typename ClassType, typename VariableType> 
	Variable::Variable(const char* name, VariableType(ClassType::*pointer)):
	Base(sizeof(*this), BaseType::Variable),
	m_visitFunction(&visitImplementation<ClassType, VariableType>),
	m_name(name)
	{
		CHECK_F(sizeof(pointer) == MemberPointerSize);
		CHECK_F(std::is_trivially_destructible<decltype(pointer)>::value);
		new(m_pointerBuffer) decltype(pointer)(pointer);
	}

	template<typename ClassType, typename VariableType, typename MetaType<VariableType>::Fundamental>
	int Variable::visitImplementation(Visitor* v, const char* name, void* voidptr, void* voidobj)
	{
		typedef VariableType (ClassType::*PtrType);
		PtrType ptr = *reinterpret_cast<PtrType*>(voidptr);
		ClassType* obj = (ClassType*)voidobj;
		return v->visit(name, (obj->*ptr));
	}

	template<typename ClassType, typename VariableType, typename MetaType<VariableType>::Array>
	int Variable::visitImplementation(Visitor* v, const char* name, void* voidptr, void* voidobj)
	{
		typedef VariableType(ClassType::*PtrType);
		PtrType ptr = *reinterpret_cast<PtrType*>(voidptr);
		ClassType* obj = (ClassType*)voidobj;
		if (v->startArray(name) == 0)
		{
			std::size_t count = 0;
			for (auto& value : (obj->*ptr))
			{
				// TODO: associative arrays
				count++;
				if (v->visit(nullptr, value) != 0)
					break;
			}

			v->endArray(count);
		}
		return true;
	}

	template<typename ClassType, typename VariableType, typename MetaType<VariableType>::Class>
	int Variable::visitImplementation(Visitor* v, const char* name, void* voidptr, void* voidobj)
	{
		typedef VariableType(ClassType::*PtrType);
		PtrType ptr = *reinterpret_cast<PtrType*>(voidptr);
		ClassType* obj = (ClassType*)voidobj;
		return Meta::visit<VariableType>(v, name, static_cast<VariableType*>(&(obj->*ptr)));
	}

	template<typename ClassType, typename VariableType, typename MetaType<VariableType>::PointerType::Object>
	static int Variable::visitImplementation(Visitor* v, const char* name, void* voidptr, void* voidobj)
	{
		typedef VariableType(ClassType::*PtrType);
		PtrType ptr = *reinterpret_cast<PtrType*>(voidptr);
		ClassType* obj = (ClassType*)voidobj;
		return v->visit(name, (obj->*ptr), getMetaSingleton<std::remove_pointer<VariableType>::type>());
	}

	template<typename ClassType, typename VariableType, typename MetaType<VariableType>::PointerType::Fundamental>
	int Variable::visitImplementation(Visitor* v, const char* name, void* voidptr, void* voidobj)
	{
		typedef VariableType(ClassType::*PtrType);
		PtrType ptr = *reinterpret_cast<PtrType*>(voidptr);
		ClassType* obj = (ClassType*)voidobj;
		return v->visit(name, (obj->*ptr));
	}

	template<typename T, typename R, typename... Args>
	std::size_t Function::sizeNeeded(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>* names, const std::array<Any, sizeof...(Args)>* defaults)
	{
		return sizeof(Function) + (names ? names->size() * sizeof(const char*) : 0) + (defaults ? defaults->size() * sizeof(Any) : 0);
	}

	template<typename T, typename R, typename... Args> 
	Function::Function(const char* name, R(T::*f)(Args...), const std::array<const char*, sizeof...(Args)>* names, const std::array<Any, sizeof...(Args)>* defaults):
	Base(Function::sizeNeeded(name, f, names, defaults), BaseType::Function),
	m_implementation(&TypeImplementation<T, R, Args...>::Instance),
	m_pointerBuffer(),
	m_name(name),
	m_argCount(names ? names->size() : 0),
	m_defaultCount(defaults ? defaults->size() : 0)
	{
		CHECK_F(sizeof(f) == MemberPointerSize);
		CHECK_F(std::is_trivially_destructible<decltype(f)>::value);
		new(m_pointerBuffer) decltype(f)(f);

		const char** bytes = (const char**)(this + 1);
		if (names)
		{
			bytes = std::copy(names->begin(), names->end(), bytes);
		}

		if (defaults)
		{
			std::copy(defaults->begin(), defaults->end(), (Any*)bytes);
		}
	}

	template<typename T, typename R, typename... Args>
	R Function::call(T* instance, Args... args)
	{
		CHECK_F(m_implementation == (&TypeImplementation<T, R, Args...>::Instance));
		return TypeImplementation<T, R, Args...>::Instance.call(instance, m_pointerBuffer, args...);
	}

	template<typename ClassType, typename R, typename... Args>
	R Function::TypeImplementation<ClassType, R, Args...>::call(ClassType* object, void* ptr, Args... args)
	{
		FuncPtr funcPtr = *((FuncPtr*)ptr);
		return (object->*funcPtr)(args...);
	}

	template<typename ClassType, typename R, typename... Args>
	int Function::TypeImplementation<ClassType, R, Args...>::visit(Visitor* v, const char* name, void* object, const char** names, Any* defaults)
	{
		int r = v->startFunction(name, std::is_void<R>::value);
		if (!std::is_void<R>::value)
		{
			const char* name = "return";
			TypeVisitor<R>::visit(v, &name, nullptr);
		}

		TypeVisitor<Args...>::visit(v, names, defaults);

		if (r == 0)
			v->endFunction();

		return r;
	}

	template<typename T, typename... Ts>
	int Function::TypeVisitor<T, Ts...>::visit(Visitor* v, const char** argNames, Any* defaults)
	{
		// TODO: feed defaults at the ending arguments, not starting arguments

		int r = 0;
		if (defaults && *defaults)
		{
			Any& t = *defaults;
			if(t.isType<int>()) r = v->visit(argNames ? *argNames : nullptr, t.get<int>());
			else if (t.isType<float>()) r = v->visit(argNames ? *argNames : nullptr, t.get<float>());
			else if (t.isType<std::string>()) r = v->visit(argNames ? *argNames : nullptr, t.get<std::string>());
			else CHECK_F(false, "Unknown default type");
		}
		else
		{
			T t{};
			r = v->visit(argNames ? *argNames : nullptr, t);
		}
		return r == 0 ? TypeVisitor<Ts...>::visit(v, argNames ? ++argNames : nullptr, defaults ? ++defaults : defaults) : r;
	}

	inline int Function::TypeVisitor<void>::visit(Visitor* v, const char** argNames, Any* defaults)
	{
		return 0;
	}

	inline int Function::TypeVisitor<>::visit(Visitor* v, const char** argNames, Any* defaults)
	{
		return 0;
	}

	template<typename T>
	Object getMeta()
	{
		static_assert("No meta data for T");
		return {};
	}

	template<typename T>
	Object& getMetaSingleton()
	{
		static Object o = getMeta<T>();
		return o;
	}

	template<typename Object>
	int visit(Visitor* v, const char* name, Object* instance)
	{
		Meta::Object& o = getMetaSingleton<Object>();
		int result = 0;
		if(v->startObject(name, o) == 0)
		{
			result = o.visit(v, instance);
			v->endObject();
		}
		return result;
	}
}