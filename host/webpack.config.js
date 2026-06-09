const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');

module.exports = {
  entry: './src/main.ts',
  module: {
    rules: [
      {
        test: /\.ts$/,
        use: 'ts-loader',
        exclude: /node_modules/,
      },
    ],
  },
  resolve: {
    extensions: ['.ts', '.js'],
  },
  output: {
    filename: 'webos-host.js',
    path: path.resolve(__dirname, '../public'),
    clean: false,
  },
  plugins: [
    new HtmlWebpackPlugin({
      title: 'WebOS',
      template: './src/index.html',
      filename: 'index.html',
    }),
  ],
  devServer: {
    static: {
      directory: path.resolve(__dirname, '../public'),
    },
    port: 8080,
    hot: true,
  },
};
