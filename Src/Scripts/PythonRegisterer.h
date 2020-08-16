#pragma once

#include "../Meta/Meta.h"
namespace Meta
{
	class PythonRegisterer : public Visitor
	{
	public:
		PythonRegisterer(Meta::Object& metaObject, const char* name, const char* doc = nullptr);
		~PythonRegisterer();

		bool addObject(PyObject* pythonModule);
		PyObject* instancePointer(void*);
		PyObject* instanceValue(const Any&);

		bool matches(const char* name) const;
		bool matches(const Meta::Object&) const;
		StringView getName() const;

		int visit(const char* name, bool&);
		int visit(const char* name, int&);
		int visit(const char* name, float&);
		int visit(const char* name, std::string&);
		int visit(const char* name, void* object, const Object&);

		int visit(const char* name, bool*);
		int visit(const char* name, int*);
		int visit(const char* name, float*);
		int visit(const char* name, std::string*);
		int visit(const char* name, void** object, const Object&);

		int startObject(const char* name, void* v, const Meta::Object& objectInfo);
		int endObject();

		int startArray(const char* name);
		int endArray(std::size_t);

		int startFunction(const char* name, bool hasReturn, bool isConstructor);
		int endFunction();

		int startFunctionObject(const char* name, bool hasReturn);
		int endFunctionObject();

		bool set(const char* name, void* instance, PyObject* value);

	protected:
		template<typename T> bool visitArg(const char* name, T& d);
		template<typename T> bool visitMember(const char* name, T& d, const Meta::Object* metaObject = nullptr);
		template<typename T> int getType(bool null = false) const;
		template<typename T> std::size_t calculateOffset(T&);
		std::size_t calculateOffset(void*);
		static char typeToFmt(int);

		struct InstanceData;
		static int pyInit(InstanceData* self, PyObject* args, PyObject* kwds);
		static void pyDealloc(PyObject* object);
		static PyObject* pyGet(PyObject* self, PyObject* attr);
		static int pySet(PyObject* self, PyObject* attr, PyObject* value);

		static PyObject* pyCallableCall(PyObject *self, PyObject *args, PyObject *kwargs);
		static void pyCallableDealloc(PyObject* object);

	protected:
		struct Member
		{
			std::string m_name;
			int m_type;
			const Meta::Object* m_metaObject;
			std::size_t m_offset;
		};
		std::vector<Member> m_members;

		struct Callable
		{
			struct FunctionObjectData;

			// Python Types
			// T_SHORT, T_INT, T_LONG, T_FLOAT, T_DOUBLE, T_STRING, T_OBJECT, T_CHAR, T_BYTE, T_UBYTE, T_USHORT
			// T_UINT, T_ULONG, T_STRING_INPLACE, T_BOOL, T_OBJECT_EX, T_LONGLONG, T_ULONGLONG, T_PYSSIZET, T_NONE
			struct Arg
			{
				std::string m_name;
				int m_type;
				Any m_default;

				std::shared_ptr<FunctionObjectData> m_funcObjData;
			};
			std::vector<Arg> m_args;
			Arg m_return;

			struct FunctionObjectData
			{
				std::vector<Arg> m_args;
				Arg m_return;
			};
			
			std::string m_name;

			bool m_isConstructor{ false };
		};

		struct InstanceData;
		struct PyCallable
		{
			PyObject_HEAD
			Callable* m_callable;
			InstanceData* m_instance;
		};
		class CallbackVisitor : public Visitor
		{
		public:
			CallbackVisitor(PyObject*);
			~CallbackVisitor();
			int visit(const char* name, bool&);
			int visit(const char* name, int&);
			int visit(const char* name, float&);
			int visit(const char* name, std::string&);
			int visit(const char* name, const char*&);
			int visit(const char* name, bool*);
			int visit(const char* name, int*);
			int visit(const char* name, float*);
			int visit(const char* name, std::string*);
			int visit(const char* name, void* object, const Object&);
			int visit(const char* name, void** object, const Object&);
			int startObject(const char* name, void* v, const Meta::Object& objectInfo);
			int endObject();
			int startArray(const char* name);
			int endArray(std::size_t);
			int startFunction(const char* name, bool hasReturn, bool isConstructor);
			int endFunction();
			int startFunctionObject(const char* name, bool hasReturn);
			int endFunctionObject();
			int startCallback(int id);
			int endCallback();

		protected:
			template<typename T> int add(char fmt, T& v);

		protected:
			PyObject* m_callback;

			static const int s_argCount = 9;
			int m_argIndex;
			char m_argFormat[s_argCount + 1];
			void* m_args[s_argCount];
			PyObject* m_needsToDec[s_argCount];
		};

		class ArgsVisitor : public Visitor
		{
		public:
			ArgsVisitor(Callable* callable, PyObject* args, PyObject* keywords);
			int visit(const char* name, bool&);
			int visit(const char* name, int&);
			int visit(const char* name, float&);
			int visit(const char* name, std::string&);
			int visit(const char* name, const char*&);
			int visit(const char* name, bool*);
			int visit(const char* name, int*);
			int visit(const char* name, float*);
			int visit(const char* name, std::string*);
			int visit(const char* name, void* object, const Object&);
			int visit(const char* name, void** object, const Object&);
			int visit(const char* name, StringView& v);
			int startObject(const char* name, void* v, const Meta::Object& objectInfo);
			int endObject();
			int startArray(const char* name);
			int endArray(std::size_t);
			int startFunction(const char* name, bool hasReturn, bool isConstructor);
			int endFunction();
			int startFunctionObject(const char* name, bool hasReturn);
			int endFunctionObject();

			std::shared_ptr<Visitor> callbackToPreviousFunctionObject(int& id);

			int getResult() const;

		protected:
			template<typename T, typename DataT = T> int visit(T&);

		protected:
			std::size_t m_argIndex;
			int m_result;

			// we store the arg data here. Max 9 of arguments, max size of a 64 bits
			std::array<void*, 9> m_data;
		};

		std::vector<Callable> m_callables;
		Callable* m_visitingFunction;
		std::shared_ptr<Callable::FunctionObjectData> m_visitingFunctionObject;
		std::string m_name;
		std::string m_nameWithModule; // Junkpile. + m_name
		std::string m_doc;
		Meta::Object& m_metaObject;
		char* m_setupHelper;

		struct InstanceData
		{
			PyObject_HEAD
			Any m_ptr; // should this be a void* instead?
			PythonRegisterer* m_class;
			bool m_owns;
		};

		PyTypeObject m_typeDef;
		PyTypeObject m_callableDef;
	};

	template<typename T> int PythonRegisterer::CallbackVisitor::add(char fmt, T& v)
	{
		// will v go out of scope?
		LOG_IF_F(FATAL, m_argIndex >= s_argCount, "Too many arguments for callback, increase s_argCount");
		m_argFormat[m_argIndex] = fmt;
		m_argFormat[m_argIndex + 1] = '\0';
		m_args[m_argIndex] = &v;
		m_argIndex++;
		return 0;
	}
}