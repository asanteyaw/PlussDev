(() => {
  const title = document.querySelector('div.body h1');
  if (!title) return;
  document.title = `${title.textContent.trim()} - PlussMa Distribution Docs`;
})();
