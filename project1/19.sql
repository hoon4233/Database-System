SELECT COUNT( DISTINCT type)
FROM Trainer, CatchedPokemon, Gym, Pokemon
WHERE Trainer.id = Gym.leader_id
and Trainer.id = CatchedPokemon.owner_id
and CatchedPokemon.pid = Pokemon.id
and Gym.city = 'Sangnok City'