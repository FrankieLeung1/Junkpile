#include "stdafx.h"
#include "NewMeta.h"
#include "../imgui/ImGuiManager.h"
#include "../Misc/Misc.h"

using namespace Meta;
std::vector<Object*> Meta::g_allMetaObjects;

Object::Object(StringView name):
m_name(name.c_str())
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

void Object::imgui()
{
	ResourcePtr<ImGuiManager> im;
	bool* opened = im->win("Meta");
	if (*opened == false)
		return;

	if (ImGui::Begin(stringf("Meta (%d)", g_allMetaObjects.size()).c_str(), opened))
	{
		for (Object* object : g_allMetaObjects)
		{
			if (ImGui::TreeNode(stringf("%s (%d)", object->getName(), object->m_members->size()).c_str()))
			{
				for (Any& member : *object->m_members)
				{
					if (auto m = member.getPtr<MemberVariable*>())
					{
						ImGui::Text((*m)->getName());
					}
					else if(auto m = member.getPtr<Function*>())
					{
						if ((*m)->m_isConstructor)
							continue;

						std::stringstream args;
						for(int i = 0; i < (*m)->m_names.size(); i++)
						{
							const char* name = (*m)->m_names[i];
							if(name)
								args << (i ? "," : "") << name;
						}

						ImGui::Text("%s(%s)", (*m)->m_name.c_str(), args.str().c_str());
						if(ImGui::IsItemHovered())
							ImGui::SetTooltip((*m)->getPtrType());
					}
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
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

Any Object::callWithVisitor(const char* name, void* instance, Visitor* v, int argCount)
{
	for (const Any& any : *m_members)
	{
		if (Function* const* m = any.getPtr<Function*>())
		{
			if ((*m)->m_name == name)
			{
				auto& r = (*m)->callWithVisitor(instance, v, argCount);
				if (!r.isType<CallFailure>())
					return r;
			}
		}
	}

	return nullptr;
}

Any Object::callWithVisitor(const char* name, Visitor* v, int argCount)
{
	for (const Any& any : *m_members)
	{
		if (Function* const* m = any.getPtr<Function*>())
		{
			if ((*m)->m_name == name)
			{
				auto& r = (*m)->callWithVisitor(v, argCount);
				if (!r.isType<CallFailure>())
					return r;
			}
		}
	}

	return nullptr;
}

Any Meta::Function::callWithVisitor(void* instance, Visitor* v, int argCount)
{
	CHECK_F(false);
	return nullptr;
}

Any Meta::Function::callWithVisitor(Visitor* v, int argCount)
{
	CHECK_F(false);
	return nullptr; 
}

void* Object::construct()
{
	return nullptr;
}

void Object::destruct(void* v)
{
	m_destructor(v);
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