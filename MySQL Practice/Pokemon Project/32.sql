SELECT Trainer.name, Pokemon.name AS Pname, COUNT(Pokemon.name) AS CaughtTimes
FROM Trainer, CatchedPokemon, Pokemon, (
  SELECT T.name AS name
  FROM Trainer AS T, CatchedPokemon AS CP, Pokemon AS P
  WHERE T.id = owner_id AND pid = P.id
  GROUP BY T.name
  HAVING COUNT(DISTINCT type)=1 ) AS OneTypeMan
WHERE Trainer.id = owner_id AND pid = Pokemon.id
AND Trainer.name = OneTypeMan.name
GROUP BY Trainer.name, Pokemon.name
ORDER BY Trainer.name
;

