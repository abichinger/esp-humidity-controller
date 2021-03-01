import Vue from 'vue'
import App from './App.vue'

import { MdButton, MdToolbar, MdIcon, MdDrawer, MdField, MdSubheader, MdDivider, MdProgress, MdSnackbar } from 'vue-material/dist/components'
import 'vue-material/dist/vue-material.min.css'

Vue.use(MdButton)
Vue.use(MdToolbar)
Vue.use(MdIcon)
Vue.use(MdDrawer)
Vue.use(MdField)
Vue.use(MdSubheader)
Vue.use(MdDivider)
Vue.use(MdProgress)
Vue.use(MdSnackbar)

Vue.config.productionTip = false

new Vue({
  render: h => h(App),
}).$mount('#app')
