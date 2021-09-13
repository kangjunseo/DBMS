# 01. Print the names of trainers that have caught 3 or more Pokémon in the order of the number of Pokémon caught.

SELECT Trainer.name
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id 
GROUP BY Trainer.name
HAVING Count(*)>=3
ORDER BY Count(*);
