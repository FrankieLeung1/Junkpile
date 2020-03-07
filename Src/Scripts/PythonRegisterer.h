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
		PyObject* instanceObject();

		bool matches(const char* name);

		int visit(const char* name, bool&);
		int visit(const char* name, int&);
		int visit(const char* name, float&);
		int visit(const char* name, std::string&);

		int visit(const char* name, bool*);
		int visit(const char* name, int*);
		int visit(const char* name, float*);
		int visit(const char* name, std::string*);
		int visit(const char* name, void* object, const Object&);

		int startObject(const char* name, const Meta::Object& objectInfo);
		int endObject();

		int startArray(const char* name);
		int endArray(std::size_t);

		int startFunction(const char* name, bool hasReturn);
		int endFunction();

	protected:
		template<typename T> bool visitArg(const char* name, T& d);
		template<typename T> bool visitMember(const char* name, T& d);
		template<typename T> int getType(bool null = false) const;

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
			std::size_t m_offset;
		};
		std::vector<Member> m_members;

		struct Callable
		{
			// Python Types
			// T_SHORT, T_INT, T_LONG, T_FLOAT, T_DOUBLE, T_STRING, T_OBJECT, T_CHAR, T_BYTE, T_UBYTE, T_USHORT
			// T_UINT, T_ULONG, T_STRING_INPLACE, T_BOOL, T_OBJECT_EX, T_LONGLONG, T_ULONGLONG, T_PYSSIZET, T_NONE
			struct Arg
			{
				std::string m_name;
				int m_type;
				Any m_default;
			};
			std::vector<Arg> m_args;
			std::string m_name;

			PyObject* m_callablePyObject;
		};

		struct InstanceData;
		struct PyCallable
		{
			PyObject_HEAD
			Callable* m_callable;
			InstanceData* m_instance;
		};
		class ArgsVisitor : public Visitor
		{
		public:
			ArgsVisitor(Callable* callable, PyObject* args, PyObject* keywords);
			int visit(const char* name, bool&);
			int visit(const char* name, int&);
			int visit(const char* name, float&);
			int visit(const char* name, std::string&);
			int visit(const char* name, bool*);
			int visit(const char* name, int*);
			int visit(const char* name, float*);
			int visit(const char* name, std::string*);
			int visit(const char* name, void* object, const Object&);
			int startObject(const char* name, const Meta::Object& objectInfo);
			int endObject();
			int startArray(const char* name);
			int endArray(std::size_t);
			int startFunction(const char* name, bool hasReturn);
			int endFunction();

		protected:
			Callable* m_callable;
			PyObject *m_args, *m_keywords;
		};

		std::map<std::string, Callable> m_callables;
		Callable* m_visitingFunction;
		std::string m_name;
		std::string m_nameWithModule; // Junkpile. + m_name
		std::string m_doc;
		Meta::Object& m_metaObject;

		struct InstanceData
		{
			PyObject_HEAD
			void* m_ptr;
			PythonRegisterer* m_class;
		};

		PyTypeObject m_typeDef;
		PyTypeObject m_callableDef;
	};
}