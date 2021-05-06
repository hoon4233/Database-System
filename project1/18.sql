SELECT AVG(level)
FROM Trainer, CatchedPokemon, Gym
WHERE Trainer.id = Gym.leader_id and Trainer.id = CatchedPokemon.owner_id