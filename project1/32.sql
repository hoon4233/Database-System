SELECT p1.name
FROM Pokemon p1
WHERE p1.id NOT IN (SELECT c2.pid FROM CatchedPokemon c2)
ORDER BY p1.name