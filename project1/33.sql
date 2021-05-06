SELECT SUM(c1.level)
FROM Trainer t1, CatchedPokemon c1
WHERE t1.id = c1.owner_id and t1.name = 'Matis'