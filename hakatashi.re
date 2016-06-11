= インターネットの1秒がもし1年だったら(仮)

//raw[|latex|\\chapterauthor{hakatashi・Mine}]

//lead{

//}

== はじめに

たぶんはじめまして。博多市(@hakatashi)です。
今回は技術書典向けの小企画として、
「インターネットの1秒がもし1年だったら」という
出落ち極まりない記事を書こうと思います。

インターネットというのは光の速さを身をもって感じることができるメディアです。
先日PHPのパッケージマネージャーであるcomposerを高速化するという内容のスライド@<fn>{composer}を
拝見したのですが、そこではcomposerが遅い原因として「光の速さが遅すぎる」というものが挙げられていました。

ユニークな考え方ですが言われてればたしかに道理で、日本からアメリカのサーバーまでハンドシェイクで
何度も往復していると、確かに光といえど100ミリ秒単位で時間を浪費しています。
サーバーが近ければ通信は早いというのは誰もが知っていることですが、
光の速度のせいだと言われるとなにやら圧倒されるものがあります。

この記事では、そんなネットワークの微視的な時間スケールについて徹底的に解剖します。
一回の通信を解析し、それぞれ時系列順に細かく分解し、それぞれの操作でどれくらいの時間が費やされ、
どんなイベントがいつ発生するのかを逐一追っていきたいと思います。

とはいえ、ネットワーク通信におけるタイムスパンはミリ秒単位で数えられます。
今回は、そんな微細な時間の移り変わりをなるべくわかりやすくするため、
@<strong>{通信上の1秒という時間を1年にまで引き伸ばし}、@<strong>{約3000万倍}の時間スケールで
ネットワークのイベントを追跡してみます。

//footnote[composer][@<href>{http://www.slideshare.net/hinakano/composer-phpstudy}]

=== 対象読者

 * ほげ
 * ほげ

=== 共著者について

=== シチュエーション

では、実際にどんな通信を解析するとよいでしょう。
徹底解剖する通信として、なるべく我々が普段慣れ親しんでいて、
それでいてそれなりに複雑で解剖しごたえのある通信を採用したいところです。

そこで、今回は@<strong>{HTTPSプロトコル}の通信をメインに解析しました。
ふだん我々がブラウザから毎日使っているプロトコルなのは言うまでもなく、
SSL/TLSの処理を挟むので、それなりに面白い結果になるのではないでしょうか。

また、HTTPS通信を行う際に必要となるARPやDNS通信についても述べていきます。

さらに、通信を行うサーバーとクライアントの場所ですが、
今回の技術書典の会場でもある東京のインターネット環境から、
さくらインターネットが誇る、石狩データセンターまで通信を行います。

Googleマップによると、東京から、石狩データセンターがある石狩市新港中央までは1177.3km。
光ファイバーに用いられる石英ガラスの屈折率は1.50程度なので、光は約20万km/sでこの距離を移動します。
つまり、東京から石狩までの物理的な片道時間は約5.9ミリ秒となります。
瞬く間もないほどの時間ですが、時間スケールを1年に引き伸ばすと2日と3時間半もかかります。
往復で4日と7時間。石狩までぶらり旅といった感じですね。

//footnote[silicon][光学ガラス材料 - シグマ光機株式会社 @<href>{http://www.products-sigmakoki.com/category/opt_d/opt_d01.html}]

==== 制約など

今回パケットを解析する上で、話を簡便化するためにいくつかの制約を加えています。

 * DNSのシステムを明確に示すため、ローカルホストにDNSサーバーを置き、
   そこからリクエストのたびにルートDNSサーバーからアドレスを引いています。
 * 時間がかかりすぎるため、DNSSECの検証は省略しています。
 * SSL/TLSの証明書検証は省略しています。

=== 測定環境

測定には、ラップトップマシン上のUbuntuの仮想環境を使用しました。
一回の測定ごとにDNSキャッシュとARPキャッシュをクリアし、
なるべくクリーンな状態で測定を行いました。

また、先程も述べたとおりDNSサーバーはローカルホストに設置し、
そこから外部に向けてアドレスを引くようにしました。

...

== ルーターのARP解決

== DNS解決

== TCP接続ハンドシェイク

== TLSネゴシエーション

== HTTPリクエストとレスポンス

== TLS終了アラート

== TCPコネクション切断

== 最後に

これは文章でした。
