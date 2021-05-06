SELECT t1.name
FROM Trainer t1, CatchedPokemon c1
WHERE t1.id = c1.owner_id 
GROUP BY t1.id, c1.pid
HAVING COUNT(c1.id)>=2
ORDER BY t1.name