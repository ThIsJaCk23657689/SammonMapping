# Scientific Visualization - Sammon Mapping

1. 將高維度資料投影到2D。
2. 設原始資料P有m筆，且每筆資料都是n維度，投影後的資料為Q，但會變成m * 2。
3. 要先計算原始資料P，每一筆資料與另一筆資料的距離d'ij，可以用一個2D array來儲存。(計算一次即可)
4. 距離的計算方法使用歐基里德距離即可。
5. 初始化Qi = [xi, yi]，先抓p資料之間的最大距離，0到100或是0到10之間隨機取數值。(記得是m筆)


進入迭代：
1. 計算每個q之間的距離
2. 計算 relative difference ： (d'ij - dij)^2/d'ij
3. 把所有的 relative difference 加總之後 如果大於一個閥值 修改每個q的位置並回到步驟2



