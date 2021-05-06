SELECT COUNT( DISTINCT CatchedPokemon.pid )
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id and Trainer.hometown = 'Sangnok City'