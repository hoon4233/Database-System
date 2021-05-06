SELECT Pokemon.name
FROM Pokemon, CatchedPokemon
WHERE Pokemon.id = CatchedPokemon.pid
and CatchedPokemon.nickname LIKE '% %'
ORDER BY Pokemon.name DESC