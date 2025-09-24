#pragma once

#include <QObject>
#include <QString>

class TAEntity : public QObject
{
    Q_OBJECT
        Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)

public:
    explicit TAEntity(QObject* parent = nullptr);

    int id() const;
    QString name() const;
    QString description() const;

public slots:
    void setId(int id);
    void setName(const QString& name);
    void setDescription(const QString& description);

signals:
    void idChanged(int id);
    void nameChanged(const QString& name);
    void descriptionChanged(const QString& description);

private:
    int m_id;
    QString m_name;
    QString m_description;
};

class TASchool : public TAEntity
{
    Q_OBJECT
        Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
        Q_PROPERTY(QString phone READ phone WRITE setPhone NOTIFY phoneChanged)

public:
    explicit TASchool(QObject* parent = nullptr);

    QString address() const;
    QString phone() const;

public slots:
    void setAddress(const QString& address);
    void setPhone(const QString& phone);

signals:
    void addressChanged(const QString& address);
    void phoneChanged(const QString& phone);

private:
    QString m_address;
    QString m_phone;
};

class TAClassroom : public TAEntity
{
    Q_OBJECT
        Q_PROPERTY(int capacity READ capacity WRITE setCapacity NOTIFY capacityChanged)
        Q_PROPERTY(TASchool* school READ school WRITE setSchool NOTIFY schoolChanged)

public:
    explicit TAClassroom(QObject* parent = nullptr);

    int capacity() const;
    TASchool* school() const;

public slots:
    void setCapacity(int capacity);
    void setSchool(TASchool* school);

signals:
    void capacityChanged(int capacity);
    void schoolChanged(TASchool* school);

private:
    int m_capacity;
    TASchool* m_school;
};

class TATeacher : public TAEntity
{
    Q_OBJECT
        Q_PROPERTY(QString subject READ subject WRITE setSubject NOTIFY subjectChanged)
        Q_PROPERTY(TAClassroom* classroom READ classroom WRITE setClassroom NOTIFY classroomChanged)
        Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)

public:
    explicit TATeacher(QObject* parent = nullptr);

    QString subject() const;
    TAClassroom* classroom() const;
    QString icon() const;

public slots:
    void setSubject(const QString& subject);
    void setClassroom(TAClassroom* classroom);
    void setIcon(const QString& icon);

signals:
    void subjectChanged(const QString& subject);
    void classroomChanged(TAClassroom* classroom);
    void iconChanged(const QString& icon);

private:
    QString m_subject;
    TAClassroom* m_classroom;
    QString m_icon;
};
class TAIMMessage : public QObject
{
    Q_OBJECT

    enum MessageType {
        VOICE = 0,
        TEXT
    };
public:
    TAIMMessage(TATeacher* teacher,const QString& content,const MessageType type,QObject* parent = nullptr);
    ~TAIMMessage();
    QString content() const;
    MessageType type() const;
    TATeacher* teacher();
    
private:
    QString m_content;
    MessageType m_type;
    TATeacher* m_teacher;
};
