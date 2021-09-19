SELECT hometown
FROM Trainer GROUP BY hometown
HAVING COUNT(*) = (
  SELECT MAX(cnt)
  FROM 
    (SELECT T.hometown, COUNT(*)AS cnt
    FROM Trainer AS T
    GROUP BY T.hometown
    ) AS Counts
  )
;

