SELECT AVG(level)
FROM Trainer, CatchedPokemon, Pokemon
WHERE Trainer.id = CatchedPokemon.owner_id and CatchedPokemon.pid = Pokemon.id
and Trainer.hometown = 'Sangnok City' and Pokemon.type = 'Electric'