SELECT DISTINCT name, type
FROM Pokemon, CatchedPokemon
WHERE Pokemon.id = CatchedPokemon.pid and CatchedPokemon.level >= 30
ORDER BY name