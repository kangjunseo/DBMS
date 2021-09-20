SELECT name, COUNT(*) AS CaughtPK
FROM Trainer, CatchedPokemon
WHERE Trainer.id = owner_id
GROUP BY name
ORDER BY name
;
