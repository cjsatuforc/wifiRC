#include "parser.h"

#include <stack>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

enum object_type
{
    LONG,
    STRING,
    CONTAINER,
};

class object
{
    public:
        object(const char* name, object_type type)
        {
            memset(m_attribute, '\0', MAX_BUFFER_SIZE);
            memcpy(m_attribute, name, strlen(name));
            m_type = type;
        }
        object_type get_type(){ return m_type; }
        const char* get_name(){ return m_attribute; };
        virtual long get_long_value(){ assert(0); };
        virtual const char* get_string_value(){ assert(0); };

    private:
        char m_attribute[MAX_BUFFER_SIZE];
        object_type m_type;
};

class long_object: public object
{
    public:
        long_object(const char* name):object(name, LONG){ }
        void set_value(long value){ m_value = value; }
        long get_long_value(){ return m_value; };

    private:
        long m_value;
};

class string_object: public object
{
    public:
        string_object(const char* name):object(name, STRING){ memset(m_value, '\0', MAX_BUFFER_SIZE); }
        void add(char c){ m_value[strlen(m_value)] = c; }
        const char* get_string_value(){ return m_value; };

    private:
        char m_value[MAX_BUFFER_SIZE];
};

class container_object: public object
{
    public:
        container_object(const char* name):object(name, CONTAINER){ }
       ~container_object()
        {
            for(int i = 0; i < m_objects.size(); i++)
            {
                delete m_objects.at(i);
            }
        }
        void add(object* object){ m_objects.push_back(object); }
        int child_count(){ return m_objects.size(); }
        object* at(int index){ return m_objects.at(index); }

    private:
        std::vector<object*> m_objects;
};

void command::populate(container_object* obj)
{
    for(int i = 0; i < obj->child_count(); i++)
    {
        object* child = obj->at(i);
        switch(child->get_type())
        {
            case STRING:
                assert(!strcmp("name", child->get_name()));
                memcpy(m_name, child->get_string_value(), strlen(child->get_string_value()));
                break;
                    case CONTAINER:
                assert(!strcmp("parameters", child->get_name()));
                container_object* container = (container_object*)child;
                populate_specific(container);
                break;
        }
        delete child;
    }
}

void set_command::populate_specific(container_object* obj)
{
    assert(!strcmp("parameters", obj->get_name()));
    container_object* container = (container_object*)obj;
    for(int i = 0; i < container->child_count(); i++)
    {
        object* param = container->at(i);
        const char* name = param->get_name();
        if(!strcmp(name, "_direction"))
        {
            m_direction = param->get_long_value();
        }
        else if(!strcmp(name, "_gear"))
        {
            m_gear = param->get_long_value();
        }
        else if(!strcmp(name, "_magnitude"))
        {
            m_magnitude = param->get_long_value();
        }
        else if(!strcmp(name, "_directionString"))
        {
            memcpy(m_direction_string, param->get_string_value(), strlen(param->get_string_value()));
        }
        else if(!strcmp(name, "_lights"))
        {
            m_lights = param->get_long_value();
        }
        else
        {
            assert(0);
        }
    }
}

enum parse_state
{
    UNKNOWN,
    ATTRIBUTE_NAME,
    VALUE
};

extern "C" {

void parse(const char* json, command* wifiRC_command)
{
    int i = 0;
    object* temp = NULL;
    char current = json[i];
    parse_state state = UNKNOWN;
    char attribute[MAX_BUFFER_SIZE];
    std::stack<container_object*> object_stack;
    while(current != '\0')
    {
        if(current == '{')
        {
            switch(state)
            {
                case UNKNOWN:
                case VALUE:
                    object_stack.push(new container_object(attribute));
                    memset(attribute, '\0', MAX_BUFFER_SIZE);
                    state = ATTRIBUTE_NAME;
                    break;
                case ATTRIBUTE_NAME:
                    assert(0);
                    break;
            }
        }
        else if(current == '}')
        {
            if(temp != NULL)
            {
                object_stack.top()->add(temp);
            }
            memset(attribute, '\0', MAX_BUFFER_SIZE);
            temp = NULL;
            container_object* obj = object_stack.top();
            object_stack.pop();
            if(!object_stack.empty())
            {
                object_stack.top()->add(obj);
            }
            else
            {
                //Top level Weave object
                wifiRC_command->populate(obj);
            }
        }
        else if(current == ':')
        {
            switch(state)
            {
                case ATTRIBUTE_NAME:
                    state = VALUE;
                    break;
                case UNKNOWN:
                case VALUE:
                    assert(0);
            }
        }
        else if(current == ',')
        {
            switch(state)
            {
                case VALUE:
                    object_stack.top()->add(temp);
                    memset(attribute, '\0', MAX_BUFFER_SIZE);
                    temp = NULL;
                    state = ATTRIBUTE_NAME;
                    break;
                case UNKNOWN:
                case ATTRIBUTE_NAME:
                    assert(0);
            }
        }
        else if(current == '"')
        {
            switch(state)
            {
                case ATTRIBUTE_NAME:
                    break;
                case VALUE:
                    if(temp == NULL)
                    {
                        temp = new string_object(attribute);
                    }
                    break;
                case UNKNOWN:
                    assert(0);
            }
        }
        else
        {
            switch(state)
            {
                case ATTRIBUTE_NAME:
                    attribute[strlen(attribute)] = current;
                    break;
                case VALUE:
                    if(temp == NULL)
                    {
                        long_object* obj = new long_object(attribute);
                        obj->set_value(atoi(json+i)); //HACK ALERT
                        temp = obj;
                    }
                    switch(temp->get_type())
                    {
                        case STRING:
                            ((string_object*)temp)->add(current);
                            break;
                    }
                    break;
                case UNKNOWN:
                    assert(0);
            }
        }
        current = json[++i];
    }

    assert(temp == NULL);
    assert(object_stack.size() == 0);

}

}
