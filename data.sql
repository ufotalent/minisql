
create table student (
	sno char(8),
	sname char(16) unique,
	sage int,
	sgender char (1),
	primary key ( sage )
);
create index index1 on student (sno);
create index index1 on student (sname);
insert into student values ('123456','a',27,'F');
insert into student values ('12345','b',28,'M');
insert into student values ('12345678','c',29,'M');
insert into student values ('1234567','d',30,'M');
insert into student values ('123456','e',31,'F');
insert into student values ('12345','f',32,'M');
insert into student values ('12345678','g',21,'M');
insert into student values ('1234567','h',22,'M');
insert into student values ('123456','i',23,'F');
insert into student values ('12345','j',24,'M');
insert into student values ('12345678','k',25,'M');
insert into student values ('1234567','l',26,'M');

select * from student;
select * from student where sname > 'c' and sage < 25;
select * from student where sage < 24 ;
select * from student where sage < 24 and sgender <> 'F';
delete from student where sage < 24 and sgender <> 'F';
select * from student where sage < 24 ;
drop table student;
quit;
