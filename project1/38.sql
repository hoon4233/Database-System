SELECT p1.name
FROM Pokemon p1 JOIN Evolution e1 ON p1.id = e1.after_id LEFT OUTER JOIN Evolution e2 ON e1.after_id = e2.before_id
WHERE e2.before_id is NULL
ORDER BY p1.name