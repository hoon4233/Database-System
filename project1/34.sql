SELECT p1.name, c1.level, c1.nickname
FROM Gym g1, Trainer t1, CatchedPokemon c1, Pokemon p1
WHERE g1.leader_id = t1.id and t1.id = c1.owner_id
and c1.pid = p1.id
and c1.nickname LIKE 'A%'
ORDER BY p1.name DESC