SELECT DISTINCT Trainer.name
FROM City, Trainer, Gym
WHERE hometown = City.name 
AND description = 'Amazon'
AND leader_id = id
;
