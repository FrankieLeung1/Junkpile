#include "stdafx.h"
#include "NewMeta.h"

using namespace Meta;

Object::Object(StringView name)
{
	auto deleter = [](std::vector<Any>* members)
	{
		for (Any& member : *members)
		{
			if (MemberVariable* const* m = member.getPtr<MemberVariable*>())
				delete *m;
			else if (auto f = member.getPtr<Function*>())
				delete *f;
		}
		delete members;
	};

	m_members = std::shared_ptr<std::vector<Any>>(new std::vector<Any>(), deleter);
}

Object::~Object()
{
	
}

const char* Object::getName() const
{
	return m_name.c_str();
}

std::size_t Object::getSize() const
{
	return m_size;
}

void Object::setSize(std::size_t s)
{
	m_size = s;
}

Object& Object::hook(const BasicFunction<int, Visitor*, void*>&)
{
	return *this;
}

Object& Object::factory(BasicFunction<void*> constructor, BasicFunction<void, void*> destructor)
{
	return *this;
}

template<typename T> bool visitHelper(Visitor* v, void* object, MemberVariable* var, int* result)
{
	if (var->getTypeInstance() == &Type<T>::s_instance)
	{
		*result = v->visit(var->getName(), *(T*)var->get(object));
		return true;
	}
	return false;
}

int Object::visit(Visitor* v, void* object) const
{
	int r = 0;
	for (const Any& any : *m_members)
	{
		if (r != 0) return r;

		if (MemberVariable* const * m = any.getPtr<MemberVariable*>())
		{
			if (visitHelper<bool>(v, object, *m, &r)) continue;
			else if (visitHelper<int>(v, object, *m, &r)) continue;
			else if (visitHelper<float>(v, object, *m, &r)) continue;
			else if (visitHelper<std::string>(v, object, *m, &r)) continue;
			else if (visitHelper<StringView>(v, object, *m, &r)) continue;
			else if (visitHelper<bool*>(v, object, *m, &r)) continue;
			else if (visitHelper<int*>(v, object, *m, &r)) continue;
			else if (visitHelper<float*>(v, object, *m, &r)) continue;
			else if (visitHelper<std::string*>(v, object, *m, &r)) continue;
			else if (visitHelper<const char*>(v, object, *m, &r)) continue;
			else
			{
				const Object* meta = (*m)->getMeta();
				CHECK_F(meta != nullptr);

				if (r = v->startObject((*m)->getName(), (*m)->get(object), *meta))
				{
					// TODO: visit all the objects in this object (maybe?)
					v->endObject();
				}
			}
		}
		else if (auto f = any.getPtr<Function*>())
		{
			r = (*f)->visit(v);
		}
	}

	return 0;
}

Any Object::callWithVisitor(const char* name, void* instance, Visitor* v)
{
	for (const Any& any : *m_members)
	{
		if (Function* const* m = any.getPtr<Function*>())
		{
			if ((*m)->m_name == name)
			{
				return (*m)->callWithVisitor(instance, v);
			}
		}
	}

	return nullptr;
}

Any Object::callWithVisitor(const char* name, Visitor* v)
{
	return nullptr;
}

Any Meta::Function::callWithVisitor(void* instance, Visitor* v) 
{
	return nullptr; 
}

Any Meta::Function::callWithVisitor(Visitor* v)
{
	return nullptr; 
}

void* Object::construct()
{
	return nullptr;
}

void Object::destruct(void*)
{

}

void Object::copyTo(void* dest, void* src) const
{

}

template<> Object Meta::instanceMeta<glm::vec4>()
{
	return Object("glm::vec4")
		.defaultFactory<glm::vec4>()
		.constructor<glm::vec4, float, float, float, float>()
		.copyOperator<glm::vec4>()
		.var("x", &glm::vec4::x)
		.var("y", &glm::vec4::y)
		.var("z", &glm::vec4::z)
		.var("w", &glm::vec4::w);
}

template<> Object Meta::instanceMeta<glm::vec3>()
{
	return Object("glm::vec3")
		.defaultFactory<glm::vec3>()
		.constructor<glm::vec3, float, float, float>()
		.copyOperator<glm::vec3>()
		.var("x", &glm::vec3::x)
		.var("y", &glm::vec3::y)
		.var("z", &glm::vec3::z);
}

template<> Object Meta::instanceMeta<glm::vec2>()
{
	return Object("glm::vec2")
		.defaultFactory<glm::vec2>()
		.constructor<glm::vec2, float, float>()
		.copyOperator<glm::vec2>()
		.var("x", &glm::vec2::x)
		.var("y", &glm::vec2::y);
}

template<> Object Meta::instanceMeta<glm::quat>()
{
	return Object("glm::quat")
		.defaultFactory<glm::quat>()
		.constructor<glm::quat, float, float, float, float>()
		.copyOperator<glm::quat>()
		.var("x", &glm::quat::x)
		.var("y", &glm::quat::y)
		.var("z", &glm::quat::z)
		.var("w", &glm::quat::w);
}