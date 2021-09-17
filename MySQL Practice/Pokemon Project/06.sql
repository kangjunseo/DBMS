SELECT name , AVG(level)
FROM Trainer, Gym, CatchedPokemon
WHERE Trainer.id = leader_id AND Trainer.id = owner_id
GROUP BY name
ORDER BY name
;
