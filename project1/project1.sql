select name
from Pokemon
where type = 'Grass'
order by name;

select name
from Trainer
where hometown = 'Brown City'
   or hometown = 'Rainbow City'
order by name;

select distinct type
from Pokemon
order by type;

select name
from City
where name like 'B%'
order by name;

select hometown
from Trainer
where name not like 'M%'
order by hometown;

select nickname
from CatchedPokemon
where level = (select max(level) from CatchedPokemon)
order by nickname;

select name
from Pokemon
where name regexp '^[AEIOU]'
order by name;

select avg(level)
from CatchedPokemon;

select max(cp.level)
from CatchedPokemon cp
         join Trainer t on cp.owner_id = t.id
where t.name = 'Yellow';

select distinct hometown
from Trainer
order by hometown;

select t.name, cp.nickname
from CatchedPokemon cp
         join Trainer t on cp.owner_id = t.id
where cp.nickname like 'A%'
order by t.name;

select t.name
from City c
         join Gym g on c.name = g.city
         join Trainer t on c.name = t.hometown and g.leader_id = t.id
where c.description = 'Amazon';

select t.id, count(*)
from CatchedPokemon cp
         join Pokemon p on cp.pid = p.id
         join Trainer t on cp.owner_id = t.id
where p.type = 'Fire'
group by t.id
order by count(*) desc
limit 1;

select distinct type
from Pokemon
where id < 10
order by id desc;

select count(*)
from Pokemon
where type <> 'Fire';

select p.name
from Evolution e
         join Pokemon p on e.before_id = p.id
where before_id > after_id
order by p.name;

select avg(level)
from CatchedPokemon cp
         join Pokemon p on cp.pid = p.id
where p.type = 'Water';

select cp.nickname
from Gym g
         join CatchedPokemon cp on g.leader_id = cp.owner_id
order by cp.level desc
limit 1;

select t.name
from CatchedPokemon cp
         join Trainer t on cp.owner_id = t.id
where t.hometown = 'Blue city'
group by t.id
having avg(cp.level) = (select max(average)
                        from (select avg(cp.level) as average
                              from CatchedPokemon cp
                                       join Trainer t on cp.owner_id = t.id
                              where t.hometown = 'Blue city'
                              group by t.id) tmp)
order by t.name;

select p.name
from CatchedPokemon cp
         join (select id from Trainer group by hometown having count(*) = 1) t on cp.owner_id = t.id
         join Pokemon p on cp.pid = p.id
         join Evolution e on p.id = e.before_id
where p.type = 'Electric';

select t.name, sum(cp.level)
from Trainer t
         join Gym g on t.id = g.leader_id
         join CatchedPokemon cp on t.id = cp.owner_id
group by t.id
order by sum(cp.level) desc;

select hometown
from Trainer
group by hometown
having count(*) = (select max(cnt) from (select count(*) as cnt from Trainer group by hometown) tmp);

select distinct p.name
from CatchedPokemon cp
         join Trainer t on cp.owner_id = t.id
         join Pokemon p on cp.pid = p.id
where t.hometown = 'Sangnok City'
  and cp.pid in (select cp.pid
                 from CatchedPokemon cp
                          join Trainer t on cp.owner_id = t.id
                 where t.hometown = 'Brown City');

select t.name
from CatchedPokemon cp
         join Trainer t on cp.owner_id = t.id
         join Pokemon p on cp.pid = p.id
where p.name like 'P%'
  and t.hometown = 'Sangnok City'
order by t.name;

select t.name as owner_name, p.name as pname
from CatchedPokemon cp
         join Trainer t on cp.owner_id = t.id
         join Pokemon p on cp.pid = p.id
order by t.name, p.name;

select p.name
from Evolution e
         join Pokemon p on e.before_id = p.id
where e.after_id not in (select before_id from Evolution)
  and e.before_id not in (select after_id from Evolution)
order by p.name;

select cp.nickname
from Gym g
         join Trainer t on g.leader_id = t.id
         join CatchedPokemon cp on t.id = cp.owner_id
         join Pokemon p on cp.pid = p.id
where g.city = 'Sangnok City'
  and p.type = 'Water'
order by cp.nickname;

select t.name
from Trainer t
         join CatchedPokemon cp on t.id = cp.owner_id
         join Evolution e on cp.pid = e.after_id
group by t.id
having count(*) >= 3
order by t.name;

select name
from Pokemon
where id not in (select pid from CatchedPokemon)
order by name;

select max(level)
from Trainer t
         join CatchedPokemon cp on t.id = cp.owner_id
group by t.hometown
order by max(level) desc;

select p1.id, p1.name as first, p2.name as second, p3.name as third
from Evolution e1
         join Evolution e2 on e1.after_id = e2.before_id
         join Pokemon p1 on e1.before_id = p1.id
         join Pokemon p2 on e2.before_id = p2.id
         join Pokemon p3 on e2.after_id = p3.id
order by p1.id;
