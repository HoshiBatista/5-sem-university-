-- Таблица студентов с вычисляемым возрастом
CREATE TABLE students (
    student_id SERIAL PRIMARY KEY,
    full_name VARCHAR(100) NOT NULL,
    birth_year INTEGER NOT NULL CHECK (birth_year > 1900 AND birth_year <= EXTRACT(YEAR FROM CURRENT_DATE)),
    age INTEGER, -- Вычисляемый столбец
    address VARCHAR(200) NOT NULL
);

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

-- Таблица групп (4NF)
CREATE TABLE groups (
    group_id VARCHAR(10) PRIMARY KEY,
    group_name VARCHAR(50)
);

-- Таблица связи групп и курсов (многие-ко-многим)
CREATE TABLE group_courses (
    group_id VARCHAR(10) NOT NULL,
    course_id INTEGER NOT NULL,
    PRIMARY KEY (group_id, course_id),
    FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE,
    FOREIGN KEY (course_id) REFERENCES courses(course_id) ON DELETE CASCADE
);

-- Таблица связи групп и преподавателей (многие-ко-многим)
CREATE TABLE group_teachers (
    group_id VARCHAR(10) NOT NULL,
    teacher_id INTEGER NOT NULL,
    PRIMARY KEY (group_id, teacher_id),
    FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE,
    FOREIGN KEY (teacher_id) REFERENCES teachers(teacher_id) ON DELETE CASCADE
);

-- Таблица связи студентов и групп
CREATE TABLE student_groups (
    student_id INTEGER NOT NULL,
    group_id VARCHAR(10) NOT NULL,
    enrollment_date DATE DEFAULT CURRENT_DATE,
    PRIMARY KEY (student_id, group_id),
    FOREIGN KEY (student_id) REFERENCES students(student_id) ON DELETE CASCADE,
    FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE
);

-- Функция для вычисления возраста
CREATE OR REPLACE FUNCTION calculate_student_age()
RETURNS TRIGGER AS $$
BEGIN
    NEW.age := DATE_PART('year', AGE(CURRENT_DATE, MAKE_DATE(NEW.birth_year, 1, 1)));
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Триггер для автоматического вычисления возраста
CREATE TRIGGER trigger_calculate_age
    BEFORE INSERT OR UPDATE OF birth_year ON students
    FOR EACH ROW
    EXECUTE FUNCTION calculate_student_age();