SELECT CP.nickname
FROM Trainer AS T, CatchedPokemon AS CP,(
  SELECT hometown, max(level) AS ML
  FROM Trainer, CatchedPokemon
  WHERE Trainer.id = owner_id
  GROUP BY hometown
  ) AS maxPK
WHERE T.id = CP.owner_id
  AND T.hometown = maxPK.hometown
  AND CP.level = maxPK.ML
ORDER BY T.hometown
;
