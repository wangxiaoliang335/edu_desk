
#include "TAEntity.h"
TAEntity::TAEntity(QObject* parent)
    : QObject(parent), m_id(-1)
{
}

int TAEntity::id() const { return m_id; }
QString TAEntity::name() const { return m_name; }
QString TAEntity::description() const { return m_description; }

void TAEntity::setId(int id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

void TAEntity::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

void TAEntity::setDescription(const QString& description)
{
    if (m_description != description) {
        m_description = description;
        emit descriptionChanged(m_description);
    }
}
TASchool::TASchool(QObject* parent)
    : TAEntity(parent)
{
}

QString TASchool::address() const { return m_address; }
QString TASchool::phone() const { return m_phone; }

void TASchool::setAddress(const QString& address)
{
    if (m_address != address) {
        m_address = address;
        emit addressChanged(m_address);
    }
}

void TASchool::setPhone(const QString& phone)
{
    if (m_phone != phone) {
        m_phone = phone;
        emit phoneChanged(m_phone);
    }
}

TAClassroom::TAClassroom(QObject* parent)
    : TAEntity(parent), m_capacity(0), m_school(nullptr)
{
}

int TAClassroom::capacity() const { return m_capacity; }
TASchool* TAClassroom::school() const { return m_school; }

void TAClassroom::setCapacity(int capacity)
{
    if (m_capacity != capacity) {
        m_capacity = capacity;
        emit capacityChanged(m_capacity);
    }
}

void TAClassroom::setSchool(TASchool* school)
{
    if (m_school != school) {
        m_school = school;
        emit schoolChanged(m_school);
    }
}
TATeacher::TATeacher(QObject* parent)
    : TAEntity(parent), m_classroom(nullptr)
{
}

QString TATeacher::subject() const { return m_subject; }
TAClassroom* TATeacher::classroom() const { return m_classroom; }
QString TATeacher::icon() const
{
    return m_icon;
}
void TATeacher::setIcon(const QString& icon)
{
    if (m_icon != icon) {
        m_icon = icon;
        emit iconChanged(m_icon);
    }
}
void TATeacher::setSubject(const QString& subject)
{
    if (m_subject != subject) {
        m_subject = subject;
        emit subjectChanged(m_subject);
    }
}

void TATeacher::setClassroom(TAClassroom* classroom)
{
    if (m_classroom != classroom) {
        m_classroom = classroom;
        emit classroomChanged(m_classroom);
    }
}
TAIMMessage::TAIMMessage(TATeacher* teacher,const QString& content, const MessageType type, QObject* parent):QObject(parent),
m_content(content),m_type(type)
{
    m_teacher = teacher;
}
TAIMMessage::~TAIMMessage() {}
QString TAIMMessage::content() const
{
    return m_content;
}
TAIMMessage::MessageType TAIMMessage::type() const
{
    return m_type;
}

TATeacher* TAIMMessage::teacher()
{
    return m_teacher;;
}
