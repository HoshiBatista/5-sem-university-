-- Таблица курсов
CREATE TABLE courses (
    course_id SERIAL PRIMARY KEY,
    course_name VARCHAR(50) NOT NULL UNIQUE
);

-- Таблица преподавателей
CREATE TABLE teachers (
    teacher_id SERIAL PRIMARY KEY,
    teacher_name VARCHAR(100) NOT NULL
);

-- Таблица преподавания (многие-ко-многим)
CREATE TABLE teaching_assignments (
    course_id INTEGER PRIMARY KEY NOT NULL,
    teacher_id INTEGER PRIMARY KEY NOT NULL,

    FOREIGN KEY (course_id) REFERENCES courses(course_id) ON DELETE CASCADE,
    FOREIGN KEY (teacher_id) REFERENCES teachers(teacher_id) ON DELETE CASCADE
);

-- Обновленная таблица групп
CREATE TABLE groups (
    group_id VARCHAR(10) PRIMARY KEY,
    course_id INTEGER NOT NULL,
    teacher_id INTEGER NOT NULL,

    FOREIGN KEY (course_id) REFERENCES courses(course_id),
    FOREIGN KEY (teacher_id) REFERENCES teachers(teacher_id)
);