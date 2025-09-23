INSERT INTO courses (course_name) VALUES 
    ('Базовый курс'),
    ('Программирование'),
    ('Подготовительный курс');

INSERT INTO teachers (teacher_name) VALUES 
    ('Дружкова Мария Владимировна'),
    ('Борисова Мария Леонидовна'),
    ('Токарев Александр Валерьевич');

INSERT INTO groups (group_id, group_name) VALUES 
    ('35', '35-я группа'),
    ('30', '30-я группа'),
    ('34', '34-я группа'),
    ('67', '67-я группа'),
    ('78', '78-я группа');

INSERT INTO group_courses (group_id, course_id) VALUES 
    ('35', 1), ('30', 2), ('34', 2), ('67', 1), ('78', 3);

INSERT INTO group_teachers (group_id, teacher_id) VALUES 
    ('35', 1), ('30', 2), ('34', 2), ('67', 3), ('78', 1);

INSERT INTO students (full_name, birth_year, address) VALUES 
    ('Уварова Мария Анатольевна', 1996, 'Гаражная 16-1'),
    ('Щербако Сергей Викторович', 2000, 'Заозерная 11'),
    ('Новицкая Любовь Валерьевна', 1998, 'Заозерная 38');

INSERT INTO student_groups (student_id, group_id) VALUES 
    (1, '35'), (2, '35'), (3, '35');

SELECT student_id, full_name, birth_year, age, address FROM students;