2025年度春学期RG講義のICARハッカソン用リポジトリ

## ファイル構成
- clustering.cpp: 異なるオブジェクトのマッチング処理（これがメイン）
- csv_scriptsディレクトリ
  - animation_cluster.py: clustering.cppの実行で得たcsvファイルから時系列でcluster_idをプロットするHTMLファイルを生成する
  - animation_object.py: clustering.cppの実行で得たcsvファイルから時系列でidをプロットするHTMLファイルを生成する
  - filter_sensors.py: 前処理用。デジタルツインのサーバーから取ってきたCSVをさらにsensor_idでフィルターする
  - remove_dimensions.py: dimensionsとdimensions_uncertaintyカラムは今回使わない上になんかJSONになってて、カンマ区切りで分類しようとした時にバグるのでカラム自体を消し去るためのスクリプト

結果はoutputディレクトリに入る。
