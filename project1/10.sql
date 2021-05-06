SELECT nickname
FROM CatchedPokemon
WHERE CatchedPokemon.level >= 50 and CatchedPokemon.owner_id >= 6
ORDER BY nickname