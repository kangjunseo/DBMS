SELECT lsum2.name
FROM (
  SELECT MAX(SL) AS lmax
  FROM (
    SELECT name, SUM(level) AS SL
    FROM Trainer, CatchedPokemon
    WHERE Trainer.id = owner_id
    GROUP BY name
    ) AS lsum 
  ) AS lsum_max, (
    SELECT name, SUM(level) AS SL
    FROM Trainer, CatchedPokemon
    WHERE Trainer.id = owner_id
    GROUP BY name
    ) AS lsum2
WHERE lsum_max.lmax = lsum2.SL
;
