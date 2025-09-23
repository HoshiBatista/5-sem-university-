-- Таблица групп (устраняет частичную зависимость)
CREATE TABLE groups (
    group_id VARCHAR(10) PRIMARY KEY,
    course VARCHAR(50) NOT NULL,
    teacher VARCHAR(100) NOT NULL
);

-- Таблица студентов (основная информация)
CREATE TABLE students (
    student_id SERIAL PRIMARY KEY,
    full_name VARCHAR(100) NOT NULL,
    birth_year INTEGER NOT NULL,
    address VARCHAR(200) NOT NULL
);

-- Таблица связи студентов и групп
CREATE TABLE student_groups (
    student_id INTEGER NOT NULL,
    group_id VARCHAR(10) NOT NULL,
    PRIMARY KEY (student_id, group_id),
    FOREIGN KEY (student_id) REFERENCES students(student_id) ON DELETE CASCADE,
    FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE
);