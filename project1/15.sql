SELECT c1.owner_id, COUNT(c1.id)
FROM CatchedPokemon c1 
GROUP BY c1.owner_id
HAVING COUNT(c1.owner_id) = (SELECT COUNT(c2.id) FROM CatchedPokemon c2 GROUP BY c2.owner_id ORDER BY COUNT(c2.id) DESC LIMIT 0, 1)
ORDER BY c1.owner_id