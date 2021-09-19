SELECT Trainer.name, nickname
FROM Trainer, CatchedPokemon,(
  SELECT name, MAX(level) AS ML
  FROM Trainer AS T, CatchedPokemon AS CP
  WHERE T.id = owner_id 
  GROUP BY name
  HAVING COUNT(*)>=4
  )AS T2
WHERE Trainer.id = owner_id
  AND T2.name = Trainer.name
  AND level = T2.ML
ORDER BY nickname
;
