SELECT Trainer.name
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id 
GROUP BY Trainer.name
HAVING Count(*)>=3
ORDER BY Count(*);
