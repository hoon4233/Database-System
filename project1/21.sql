SELECT Trainer.name, COUNT(CatchedPokemon.id)
FROM Trainer, CatchedPokemon, Gym
WHERE Trainer.id = CatchedPokemon.owner_id
and Trainer.id = Gym.leader_id
GROUP BY Trainer.name
ORDER BY Trainer.name